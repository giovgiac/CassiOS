// fs.rs -- FAT32 filesystem
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Paths use '/' separators, names are case-sensitive, and "." and ".."
// work as usual. A write replaces the whole file. Every walk along a
// cluster chain is bounded by the number of clusters, so a corrupt FAT
// cannot loop forever. Both FAT copies are written on every change.

use core::ops::ControlFlow;

use alloc::vec::Vec;

use crate::cache::{BlockDevice, SECTOR_SIZE, Sector, SectorCache};
use crate::dir::{self, ENTRY_SIZE, Entry, Kind, LongName, NAME_MAX};
use crate::layout::Layout;

const FAT_FREE: u32 = 0;
const FAT_EOC: u32 = 0x0FFF_FFF8;
const FAT_MASK: u32 = 0x0FFF_FFFF;
const FAT_ENTRIES_PER_SECTOR: u32 = (SECTOR_SIZE / 4) as u32;
const ENTRIES_PER_SECTOR: usize = SECTOR_SIZE / ENTRY_SIZE;
const ZERO_SECTOR: Sector = [0; SECTOR_SIZE];

/// Open files at once; handles are 1..=MAX_HANDLES.
pub const MAX_HANDLES: usize = 16;
const CACHE_SECTORS: usize = 16;
/// Long-name entries a name of NAME_MAX characters needs, plus its short
/// entry.
const MAX_ENTRIES_PER_NAME: usize = NAME_MAX.div_ceil(dir::LFN_CHARS) + 1;

/// Where a directory entry is on disk.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
struct Slot {
    lba: u32,
    index: usize,
}

/// What a path resolved to.
struct Node {
    entry: Entry,
    /// None for the root and for directories reached through "..".
    slot: Option<Slot>,
}

/// A visible directory entry and the entries that make up its name.
struct Found {
    entry: Entry,
    slot: Slot,
    long: [Slot; MAX_ENTRIES_PER_NAME],
    long_count: usize,
}

struct Handle {
    cluster: u32,
    size: u32,
    slot: Slot,
}

/// Type of the object at a path, as reported by `stat`.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Stat {
    Missing = 0,
    File = 1,
    Directory = 2,
}

pub struct Filesystem<D: BlockDevice> {
    cache: SectorCache<D>,
    layout: Layout,
    handles: Vec<Option<Handle>>,
}

impl<D: BlockDevice> Filesystem<D> {
    /// Mount the FAT32 volume on `device`, or None if there is none (or
    /// memory for the cache and handles cannot be allocated).
    pub fn mount(device: D) -> Option<Self> {
        let mut cache = SectorCache::new(device, CACHE_SECTORS)?;
        let layout = Layout::parse(cache.read(0)?)?;
        let mut handles = Vec::new();
        handles.try_reserve_exact(MAX_HANDLES).ok()?;
        handles.resize_with(MAX_HANDLES, || None);
        Some(Self {
            cache,
            layout,
            handles,
        })
    }

    pub fn stat(&mut self, path: &[u8]) -> Stat {
        match self.resolve(path) {
            None => Stat::Missing,
            Some(node) if node.entry.is_dir() => Stat::Directory,
            Some(_) => Stat::File,
        }
    }

    /// Name of the `index`th visible entry of the directory at `path`.
    /// `out` must hold NAME_MAX bytes.
    pub fn list(&mut self, path: &[u8], index: usize, out: &mut [u8]) -> Option<usize> {
        let node = self.resolve(path)?;
        if !node.entry.is_dir() {
            return None;
        }
        let mut len = 0;
        self.find(node.entry.cluster, |i, name, _| {
            if i == index {
                out[..name.len()].copy_from_slice(name);
                len = name.len();
            }
            i == index
        })?;
        Some(len)
    }

    pub fn mkdir(&mut self, path: &[u8]) -> bool {
        self.create(path, dir::ATTR_DIRECTORY).is_some()
    }

    /// Remove a file or an empty directory. Open handles to it stop working.
    pub fn remove(&mut self, path: &[u8]) -> bool {
        self.try_remove(path).is_some()
    }

    /// Open the file at `path`, creating it if `create` is set. Returns a
    /// handle, or 0 on failure (missing file, a directory, no free handle).
    pub fn open(&mut self, path: &[u8], create: bool) -> u32 {
        let Some(free) = self.handles.iter().position(Option::is_none) else {
            return 0;
        };
        let (entry, slot) = match self.resolve(path) {
            Some(Node {
                entry,
                slot: Some(slot),
            }) if !entry.is_dir() => (entry, slot),
            Some(_) => return 0,
            None if create => match self.create(path, dir::ATTR_ARCHIVE) {
                Some(created) => created,
                None => return 0,
            },
            None => return 0,
        };
        self.handles[free] = Some(Handle {
            cluster: entry.cluster,
            size: entry.size,
            slot,
        });
        free as u32 + 1
    }

    pub fn close(&mut self, handle: u32) -> bool {
        match self.handle_index(handle) {
            Some(i) => {
                self.handles[i] = None;
                true
            }
            None => false,
        }
    }

    /// Read from `offset` into `buf`; returns the bytes read (0 at the end
    /// of the file), or None for a bad handle or a disk error.
    pub fn read(&mut self, handle: u32, offset: u32, buf: &mut [u8]) -> Option<usize> {
        let h = self.handles[self.handle_index(handle)?].as_ref()?;
        let (start, size) = (h.cluster, h.size);
        if offset >= size {
            return Some(0);
        }
        let len = buf.len().min((size - offset) as usize);
        let cluster_bytes = self.layout.bytes_per_cluster();

        let mut cluster = start;
        for _ in 0..offset / cluster_bytes {
            cluster = self.next_cluster(cluster)?;
        }
        let mut pos = offset;
        let mut done = 0;
        while done < len {
            if !self.layout.is_data_cluster(cluster) {
                break;
            }
            let in_cluster = pos % cluster_bytes;
            let lba = self.layout.cluster_lba(cluster) + in_cluster / SECTOR_SIZE as u32;
            let in_sector = (pos % SECTOR_SIZE as u32) as usize;
            let chunk = (SECTOR_SIZE - in_sector).min(len - done);
            let sector = self.cache.read(lba)?;
            buf[done..done + chunk].copy_from_slice(&sector[in_sector..in_sector + chunk]);
            done += chunk;
            pos += chunk as u32;
            if pos % cluster_bytes == 0 && done < len {
                cluster = self.next_cluster(cluster)?;
            }
        }
        Some(done)
    }

    /// Replace the file's contents with `data`.
    pub fn write(&mut self, handle: u32, data: &[u8]) -> bool {
        self.try_write(handle, data).is_some()
    }

    // ---- paths -----------------------------------------------------------

    /// Follow `path` from the root. Intermediate components must be
    /// directories.
    fn resolve(&mut self, path: &[u8]) -> Option<Node> {
        if path.is_empty() {
            return None;
        }
        let mut node = self.root();
        for name in path.split(|&c| c == b'/') {
            match name {
                _ if !node.entry.is_dir() => return None,
                b"" | b"." => continue,
                b".." => node = self.parent_of(node.entry.cluster)?,
                _ => {
                    let found = self.find_name(node.entry.cluster, name)?;
                    node = Node {
                        entry: found.entry,
                        slot: Some(found.slot),
                    };
                }
            }
        }
        Some(node)
    }

    /// The directory holding the last component of `path`, and that name.
    fn split_parent<'a>(&mut self, path: &'a [u8]) -> Option<(u32, &'a [u8])> {
        let slash = path.iter().rposition(|&c| c == b'/')?;
        let name = &path[slash + 1..];
        if matches!(name, b"" | b"." | b"..") || name.len() > NAME_MAX {
            return None;
        }
        let parent = if slash == 0 {
            self.root()
        } else {
            self.resolve(&path[..slash])?
        };
        parent
            .entry
            .is_dir()
            .then_some((parent.entry.cluster, name))
    }

    fn root(&self) -> Node {
        Node {
            entry: Entry {
                attr: dir::ATTR_DIRECTORY,
                cluster: self.layout.root_cluster,
                ..Entry::default()
            },
            slot: None,
        }
    }

    /// The parent of the directory at `cluster`, from its ".." entry
    /// (cluster 0 there means the root).
    fn parent_of(&mut self, cluster: u32) -> Option<Node> {
        if cluster == self.layout.root_cluster {
            return Some(self.root());
        }
        let sector = self.cache.read(self.layout.cluster_lba(cluster))?;
        let dotdot = Entry::parse(&sector[ENTRY_SIZE..2 * ENTRY_SIZE]);
        let mut node = self.root();
        if dotdot.cluster != 0 {
            node.entry.cluster = dotdot.cluster;
        }
        Some(node)
    }

    // ---- directories -----------------------------------------------------

    fn find_name(&mut self, dir: u32, name: &[u8]) -> Option<Found> {
        self.find(dir, |_, entry_name, _| entry_name == name)
    }

    /// The first visible entry of the directory at `dir` for which
    /// `want(index, name, entry)` holds.
    fn find(
        &mut self,
        dir: u32,
        mut want: impl FnMut(usize, &[u8], &Entry) -> bool,
    ) -> Option<Found> {
        let mut long = LongName::new();
        let mut long_slots = [Slot::default(); MAX_ENTRIES_PER_NAME];
        let mut long_count = 0;
        let mut index = 0;
        let mut name = [0u8; NAME_MAX];
        self.walk(dir, |slot, raw| match dir::classify(raw) {
            Kind::End => ControlFlow::Break(None),
            Kind::Free | Kind::Hidden => {
                long.clear();
                long_count = 0;
                ControlFlow::Continue(())
            }
            Kind::LongName => {
                long.push(raw);
                // One place stays free for the short entry.
                if long_count < long_slots.len() - 1 {
                    long_slots[long_count] = slot;
                    long_count += 1;
                }
                ControlFlow::Continue(())
            }
            Kind::Visible(entry) => {
                let len = if long.is_empty() {
                    dir::short_name_text(&entry, &mut name)
                } else {
                    long.read_into(&mut name)
                };
                if want(index, &name[..len], &entry) {
                    return ControlFlow::Break(Some(Found {
                        entry,
                        slot,
                        long: long_slots,
                        long_count,
                    }));
                }
                index += 1;
                long.clear();
                long_count = 0;
                ControlFlow::Continue(())
            }
        })
        .flatten()
    }

    /// Visit every entry slot of the directory at `dir` in order, until
    /// `visit` breaks. None if the chain ends first or a read fails.
    fn walk<T>(
        &mut self,
        dir: u32,
        mut visit: impl FnMut(Slot, &[u8]) -> ControlFlow<T>,
    ) -> Option<T> {
        let mut cluster = dir;
        for _ in 0..self.layout.cluster_end {
            if !self.layout.is_data_cluster(cluster) {
                return None;
            }
            for s in 0..self.layout.sectors_per_cluster {
                let lba = self.layout.cluster_lba(cluster) + s;
                for index in 0..ENTRIES_PER_SECTOR {
                    let sector = self.cache.read(lba)?;
                    let raw: [u8; ENTRY_SIZE] = sector[index * ENTRY_SIZE..][..ENTRY_SIZE]
                        .try_into()
                        .unwrap();
                    if let ControlFlow::Break(t) = visit(Slot { lba, index }, &raw) {
                        return Some(t);
                    }
                }
            }
            cluster = self.next_cluster(cluster)?;
        }
        None
    }

    /// Create a file or directory at `path` (which must not exist yet).
    fn create(&mut self, path: &[u8], attr: u8) -> Option<(Entry, Slot)> {
        let (dir, name) = self.split_parent(path)?;
        if self.find_name(dir, name).is_some() {
            return None;
        }
        let short = dir::short_name(name);
        let long_count = if dir::needs_long_name(name) {
            dir::long_entries(name)
        } else {
            0
        };
        let slots = self.free_run(dir, long_count + 1)?;

        let mut entry = Entry {
            name: short,
            attr,
            ..Entry::default()
        };
        if attr & dir::ATTR_DIRECTORY != 0 {
            entry.cluster = self.new_directory(dir)?;
        }

        // Long-name entries in disk order (highest sequence first), then the
        // short entry; one write per sector they share.
        let sum = dir::checksum(&short);
        self.update_entries(&slots[..=long_count], |i, raw| {
            if i < long_count {
                dir::store_long_entry(raw, name, long_count - i, i == 0, sum);
            } else {
                entry.store(raw);
            }
        })?;
        Some((entry, slots[long_count]))
    }

    /// `count` consecutive free entry slots in the directory at `dir`,
    /// growing it by a cluster when it runs out.
    fn free_run(&mut self, dir: u32, count: usize) -> Option<[Slot; MAX_ENTRIES_PER_NAME]> {
        let mut run = [Slot::default(); MAX_ENTRIES_PER_NAME];
        let mut found = 0;
        let mut cluster = dir;
        for _ in 0..self.layout.cluster_end {
            for s in 0..self.layout.sectors_per_cluster {
                let lba = self.layout.cluster_lba(cluster) + s;
                for index in 0..ENTRIES_PER_SECTOR {
                    let first = self.cache.read(lba)?[index * ENTRY_SIZE];
                    if first == dir::END || first == dir::FREE {
                        run[found] = Slot { lba, index };
                        found += 1;
                        if found == count {
                            return Some(run);
                        }
                    } else {
                        found = 0;
                    }
                }
            }
            cluster = match self.next_cluster(cluster) {
                Some(next) => next,
                None => {
                    let next = self.allocate_zeroed()?;
                    if !self.fat_set(cluster, next) {
                        return None;
                    }
                    next
                }
            };
        }
        None
    }

    /// Allocate and initialise a directory cluster whose ".." points at
    /// `parent`.
    fn new_directory(&mut self, parent: u32) -> Option<u32> {
        let cluster = self.allocate_zeroed()?;
        let parent = if parent == self.layout.root_cluster {
            0
        } else {
            parent
        };
        let dots = [(*b".          ", cluster), (*b"..         ", parent)];
        let lba = self.layout.cluster_lba(cluster);
        let ok = self.cache.update(lba, |sector| {
            for (i, (name, target)) in dots.into_iter().enumerate() {
                let e = Entry {
                    name,
                    attr: dir::ATTR_DIRECTORY,
                    cluster: target,
                    ..Entry::default()
                };
                e.store(&mut sector[i * ENTRY_SIZE..]);
            }
        });
        ok.then_some(cluster)
    }

    fn update_entry(&mut self, slot: Slot, change: impl FnOnce(&mut [u8])) -> Option<()> {
        let offset = slot.index * ENTRY_SIZE;
        self.cache
            .update(slot.lba, |sector| {
                change(&mut sector[offset..offset + ENTRY_SIZE])
            })
            .then_some(())
    }

    /// Apply `change(i, raw)` to each of `slots`, writing each sector once.
    /// Slots are consecutive, so those in one sector are adjacent.
    fn update_entries(
        &mut self,
        slots: &[Slot],
        mut change: impl FnMut(usize, &mut [u8]),
    ) -> Option<()> {
        let mut start = 0;
        while start < slots.len() {
            let lba = slots[start].lba;
            let end = start + slots[start..].iter().take_while(|s| s.lba == lba).count();
            let ok = self.cache.update(lba, |sector| {
                for (i, slot) in slots[start..end].iter().enumerate() {
                    let offset = slot.index * ENTRY_SIZE;
                    change(start + i, &mut sector[offset..offset + ENTRY_SIZE]);
                }
            });
            if !ok {
                return None;
            }
            start = end;
        }
        Some(())
    }

    fn try_remove(&mut self, path: &[u8]) -> Option<()> {
        let (dir, name) = self.split_parent(path)?;
        let found = self.find_name(dir, name)?;
        if found.entry.is_dir() && self.find(found.entry.cluster, |_, _, _| true).is_some() {
            return None;
        }
        self.free_chain(found.entry.cluster);
        let mut slots = [Slot::default(); MAX_ENTRIES_PER_NAME];
        slots[..found.long_count].copy_from_slice(&found.long[..found.long_count]);
        slots[found.long_count] = found.slot;
        self.update_entries(&slots[..=found.long_count], |_, raw| raw[0] = dir::FREE)?;
        for h in self.handles.iter_mut() {
            if h.as_ref().is_some_and(|h| h.slot == found.slot) {
                *h = None;
            }
        }
        Some(())
    }

    // ---- files -------------------------------------------------------------

    fn handle_index(&self, handle: u32) -> Option<usize> {
        let i = (handle as usize).checked_sub(1)?;
        self.handles.get(i)?.as_ref().map(|_| i)
    }

    fn try_write(&mut self, handle: u32, data: &[u8]) -> Option<()> {
        let i = self.handle_index(handle)?;
        let h = self.handles[i].as_ref()?;
        let (old, slot) = (h.cluster, h.slot);

        let start = if data.is_empty() {
            0
        } else if self.layout.is_data_cluster(old) {
            self.write_chain(old, data)?;
            old
        } else {
            // A new chain: give it back if the write fails part way.
            let first = self.allocate()?;
            if self.write_chain(first, data).is_none() {
                self.free_chain(first);
                return None;
            }
            first
        };

        self.update_entry(slot, |raw| {
            let mut e = Entry::parse(raw);
            e.cluster = start;
            e.size = data.len() as u32;
            e.store_location(raw);
        })?;
        // Every handle to this file sees the new chain.
        for h in self.handles.iter_mut().flatten() {
            if h.slot == slot {
                h.cluster = start;
                h.size = data.len() as u32;
            }
        }
        // Emptied: the old chain goes only once the entry no longer uses it.
        if data.is_empty() {
            self.free_chain(old);
        }
        Some(())
    }

    /// Write `data` over the chain starting at `first`, extending it as
    /// needed and freeing what is left over. A corrupt chain that loops
    /// back into the clusters already written is cut there and extended.
    fn write_chain(&mut self, first: u32, data: &[u8]) -> Option<()> {
        let cluster_bytes = self.layout.bytes_per_cluster() as usize;
        let mut cluster = first;
        let mut written = 1;
        let mut chunks = data.chunks(cluster_bytes).peekable();
        while let Some(chunk) = chunks.next() {
            let lba = self.layout.cluster_lba(cluster);
            for (s, part) in chunk.chunks(SECTOR_SIZE).enumerate() {
                let lba = lba + s as u32;
                let ok = match <&Sector>::try_from(part) {
                    Ok(full) => self.cache.write(lba, full),
                    Err(_) => self
                        .cache
                        .update(lba, |sector| sector[..part.len()].copy_from_slice(part)),
                };
                if !ok {
                    return None;
                }
            }
            if chunks.peek().is_some() {
                cluster = match self.next_cluster(cluster) {
                    Some(next) if !self.in_chain(first, written, next) => next,
                    _ => {
                        let next = self.allocate()?;
                        if !self.fat_set(cluster, next) {
                            return None;
                        }
                        next
                    }
                };
                written += 1;
            }
        }
        // End the chain here and free the rest, unless it loops back.
        let rest = self.fat_get(cluster);
        if !self.fat_set(cluster, FAT_EOC) {
            return None;
        }
        if !self.in_chain(first, written, rest) {
            self.free_chain(rest);
        }
        Some(())
    }

    // ---- FAT -------------------------------------------------------------

    fn fat_position(&self, cluster: u32) -> (u32, usize) {
        let lba = self.layout.fat_start + cluster / FAT_ENTRIES_PER_SECTOR;
        (lba, (cluster % FAT_ENTRIES_PER_SECTOR) as usize * 4)
    }

    fn fat_get(&mut self, cluster: u32) -> u32 {
        if cluster >= self.layout.cluster_end {
            return FAT_EOC;
        }
        let (lba, offset) = self.fat_position(cluster);
        match self.cache.read(lba) {
            Some(sector) => {
                u32::from_le_bytes(sector[offset..offset + 4].try_into().unwrap()) & FAT_MASK
            }
            None => FAT_EOC,
        }
    }

    /// Set the FAT entry of `cluster` in every FAT copy, keeping the
    /// reserved top bits.
    fn fat_set(&mut self, cluster: u32, value: u32) -> bool {
        if cluster >= self.layout.cluster_end {
            return false;
        }
        let (lba, offset) = self.fat_position(cluster);
        (0..self.layout.num_fats).all(|copy| {
            let lba = lba + copy * self.layout.fat_size;
            let Some(sector) = self.cache.read(lba) else {
                return false;
            };
            let old = u32::from_le_bytes(sector[offset..offset + 4].try_into().unwrap());
            let new = old & !FAT_MASK | value & FAT_MASK;
            // A copy that already holds `value` is not written again.
            old == new
                || self.cache.update(lba, |sector| {
                    sector[offset..offset + 4].copy_from_slice(&new.to_le_bytes())
                })
        })
    }

    /// The cluster after `cluster` in its chain, if any.
    fn next_cluster(&mut self, cluster: u32) -> Option<u32> {
        let next = self.fat_get(cluster);
        self.layout.is_data_cluster(next).then_some(next)
    }

    /// Whether `cluster` is one of the first `len` clusters of the chain
    /// starting at `start`.
    fn in_chain(&mut self, start: u32, len: usize, cluster: u32) -> bool {
        let mut c = start;
        for _ in 0..len {
            if c == cluster {
                return true;
            }
            match self.next_cluster(c) {
                Some(next) => c = next,
                None => return false,
            }
        }
        false
    }

    /// Mark the first free cluster as the end of a chain and return it.
    fn allocate(&mut self) -> Option<u32> {
        let cluster = (2..self.layout.cluster_end).find(|&c| self.fat_get(c) == FAT_FREE)?;
        self.fat_set(cluster, FAT_EOC).then_some(cluster)
    }

    fn allocate_zeroed(&mut self) -> Option<u32> {
        let cluster = self.allocate()?;
        let lba = self.layout.cluster_lba(cluster);
        let zeroed =
            (0..self.layout.sectors_per_cluster).all(|s| self.cache.write(lba + s, &ZERO_SECTOR));
        zeroed.then_some(cluster)
    }

    fn free_chain(&mut self, start: u32) {
        let mut cluster = start;
        for _ in 0..self.layout.cluster_end {
            if !self.layout.is_data_cluster(cluster) {
                return;
            }
            let next = self.fat_get(cluster);
            self.fat_set(cluster, FAT_FREE);
            cluster = next;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::tests::MemDisk;
    use crate::layout::tests::boot_sector;

    const TOTAL: u32 = 8192;

    /// A blank FAT32 volume of TOTAL sectors (4 MiB).
    fn format(sectors_per_cluster: u8) -> MemDisk {
        let clusters = (TOTAL - 32) / sectors_per_cluster as u32;
        let fat_size = (clusters + 2).div_ceil(128);
        let mut disk = MemDisk::new(TOTAL as usize);
        disk.sectors[0] = boot_sector(TOTAL, sectors_per_cluster, fat_size);
        for copy in 0..2 {
            let fat = &mut disk.sectors[(32 + copy * fat_size) as usize];
            fat[0..4].copy_from_slice(&0x0FFF_FFF8u32.to_le_bytes());
            fat[4..8].copy_from_slice(&0x0FFF_FFFFu32.to_le_bytes());
            fat[8..12].copy_from_slice(&FAT_EOC.to_le_bytes());
        }
        disk
    }

    fn fs(sectors_per_cluster: u8) -> Filesystem<MemDisk> {
        Filesystem::mount(format(sectors_per_cluster)).unwrap()
    }

    fn names(fs: &mut Filesystem<MemDisk>, path: &[u8]) -> Vec<Vec<u8>> {
        let mut out = Vec::new();
        let mut name = [0; NAME_MAX];
        while let Some(len) = fs.list(path, out.len(), &mut name) {
            out.push(name[..len].to_vec());
        }
        out
    }

    fn write_file(fs: &mut Filesystem<MemDisk>, path: &[u8], data: &[u8]) {
        let h = fs.open(path, true);
        assert_ne!(h, 0);
        assert!(fs.write(h, data));
        assert!(fs.close(h));
    }

    fn read_file(fs: &mut Filesystem<MemDisk>, path: &[u8]) -> Vec<u8> {
        let h = fs.open(path, false);
        assert_ne!(h, 0);
        let mut buf = alloc::vec![0; 100_000];
        let n = fs.read(h, 0, &mut buf).unwrap();
        fs.close(h);
        buf.truncate(n);
        buf
    }

    fn free_clusters(fs: &mut Filesystem<MemDisk>) -> usize {
        (2..fs.layout.cluster_end)
            .filter(|&c| fs.fat_get(c) == FAT_FREE)
            .count()
    }

    fn pattern(len: usize) -> Vec<u8> {
        (0..len).map(|i| (i * 7 % 251) as u8).collect()
    }

    #[test]
    fn zeroed_disk_does_not_mount() {
        assert!(Filesystem::mount(MemDisk::new(64)).is_none());
    }

    #[test]
    fn empty_root() {
        let mut fs = fs(1);
        assert_eq!(fs.stat(b"/"), Stat::Directory);
        assert!(names(&mut fs, b"/").is_empty());
        assert_eq!(fs.stat(b""), Stat::Missing);
    }

    #[test]
    fn mkdir_and_list() {
        let mut fs = fs(1);
        assert!(fs.mkdir(b"/docs"));
        assert!(fs.mkdir(b"/docs/notes"));
        assert_eq!(names(&mut fs, b"/"), [b"docs".to_vec()]);
        assert_eq!(names(&mut fs, b"/docs"), [b"notes".to_vec()]);
        assert_eq!(fs.stat(b"/docs/notes"), Stat::Directory);
        assert!(!fs.mkdir(b"/docs"), "duplicate");
        assert!(!fs.mkdir(b"/missing/dir"), "missing parent");
        assert!(!fs.mkdir(b"/"));
    }

    #[test]
    fn dot_names_cannot_be_created() {
        let mut fs = fs(1);
        assert!(fs.mkdir(b"/a"));
        for path in [&b"/."[..], b"/..", b"/a/.", b"/a/..", b"/a/"] {
            assert!(!fs.mkdir(path));
            assert_eq!(fs.open(path, true), 0);
        }
        assert_eq!(names(&mut fs, b"/"), [b"a".to_vec()]);
        assert!(names(&mut fs, b"/a").is_empty());
    }

    #[test]
    fn trailing_and_leading_dot_names_are_found_again() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/NOTE.", b"1");
        write_file(&mut fs, b"/.TXT", b"2");
        write_file(&mut fs, b"/NOTE.", b"3");
        assert_eq!(names(&mut fs, b"/"), [b"NOTE.".to_vec(), b".TXT".to_vec()]);
        assert_eq!(read_file(&mut fs, b"/NOTE."), b"3");
        assert_eq!(read_file(&mut fs, b"/.TXT"), b"2");
    }

    #[test]
    fn longest_names_list_in_full() {
        let mut fs = fs(1);
        let mut path = b"/".to_vec();
        path.extend_from_slice(&[b'n'; NAME_MAX]);
        write_file(&mut fs, &path, b"x");
        assert_eq!(names(&mut fs, b"/"), [path[1..].to_vec()]);
        assert!(!fs.mkdir(&[&path[..], b"n"].concat()), "over NAME_MAX");
    }

    #[test]
    fn create_write_read() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/hello.txt", b"Hello, World!");
        assert_eq!(read_file(&mut fs, b"/hello.txt"), b"Hello, World!");
        assert_eq!(fs.stat(b"/hello.txt"), Stat::File);
        assert_eq!(names(&mut fs, b"/"), [b"hello.txt".to_vec()]);
    }

    #[test]
    fn upper_case_8_3_names_are_found_after_creation() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/NOTE.TXT", b"x");
        assert_eq!(names(&mut fs, b"/"), [b"NOTE.TXT".to_vec()]);
        assert_eq!(read_file(&mut fs, b"/NOTE.TXT"), b"x");
    }

    #[test]
    fn large_files_span_clusters() {
        for spc in [1, 4] {
            let mut fs = fs(spc);
            let data = pattern(5000);
            write_file(&mut fs, b"/big", &data);
            assert_eq!(read_file(&mut fs, b"/big"), data);
        }
    }

    #[test]
    fn read_with_offset() {
        let mut fs = fs(1);
        let data = pattern(1500);
        write_file(&mut fs, b"/f", &data);
        let h = fs.open(b"/f", false);
        let mut buf = [0; 100];
        assert_eq!(fs.read(h, 1000, &mut buf), Some(100));
        assert_eq!(&buf[..], &data[1000..1100]);
        assert_eq!(fs.read(h, 1450, &mut buf), Some(50));
        assert_eq!(fs.read(h, 1500, &mut buf), Some(0));
    }

    #[test]
    fn overwrite_shorter_frees_clusters() {
        let mut fs = fs(1);
        let before = free_clusters(&mut fs);
        write_file(&mut fs, b"/f", &pattern(3000));
        write_file(&mut fs, b"/f", b"short");
        assert_eq!(read_file(&mut fs, b"/f"), b"short");
        assert_eq!(free_clusters(&mut fs), before - 1);
        write_file(&mut fs, b"/f", b"");
        assert_eq!(read_file(&mut fs, b"/f"), b"");
        assert_eq!(free_clusters(&mut fs), before);
    }

    #[test]
    fn remove_returns_every_cluster() {
        let mut fs = fs(1);
        let before = free_clusters(&mut fs);
        assert!(fs.mkdir(b"/d"));
        write_file(&mut fs, b"/d/a-long-file-name.txt", &pattern(2000));
        assert!(!fs.remove(b"/d"), "not empty");
        assert!(fs.remove(b"/d/a-long-file-name.txt"));
        assert!(fs.remove(b"/d"));
        assert_eq!(free_clusters(&mut fs), before);
        assert!(names(&mut fs, b"/").is_empty());
        assert!(!fs.remove(b"/"));
        assert!(!fs.remove(b"/d"));
    }

    #[test]
    fn open_rules() {
        let mut fs = fs(1);
        assert_eq!(fs.open(b"/missing", false), 0);
        assert!(fs.mkdir(b"/d"));
        assert_eq!(fs.open(b"/d", false), 0, "directories cannot be opened");
        assert_eq!(fs.open(b"/", false), 0);
        assert_eq!(fs.open(b"/d/new", true), 1);
    }

    #[test]
    fn handles_are_reused_after_close() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", b"x");
        for _ in 0..100 {
            let h = fs.open(b"/f", false);
            assert_ne!(h, 0);
            assert!(fs.close(h));
        }
        let open: Vec<u32> = (0..MAX_HANDLES).map(|_| fs.open(b"/f", false)).collect();
        assert!(open.iter().all(|&h| h != 0));
        assert_eq!(fs.open(b"/f", false), 0, "table full");
        assert!(!fs.close(0));
        assert!(!fs.close(MAX_HANDLES as u32 + 1));
    }

    #[test]
    fn writes_are_seen_by_every_handle_to_the_file() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", &pattern(1500));
        let (h1, h2) = (fs.open(b"/f", false), fs.open(b"/f", false));
        assert!(fs.write(h1, b""));
        write_file(&mut fs, b"/g", &pattern(1500));
        // h2 must not write over g's clusters with f's old chain.
        assert!(fs.write(h2, b"x"));
        assert_eq!(read_file(&mut fs, b"/g"), pattern(1500));
        assert_eq!(read_file(&mut fs, b"/f"), b"x");
        let mut buf = [0; 4];
        assert_eq!(fs.read(h1, 0, &mut buf), Some(1));
    }

    #[test]
    fn writes_retried_after_a_disk_error_reach_the_disk() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", &pattern(3000));
        let h = fs.open(b"/f", false);
        let first = fs.handles[h as usize - 1].as_ref().unwrap().cluster;
        // The first FAT sector fails once, when the chain is cut short.
        fs.cache.device().fail_lba = Some(fs.layout.fat_start);
        assert!(!fs.write(h, b"short"));
        fs.cache.device().fail_lba = None;
        assert!(fs.write(h, b"short"));
        let before = free_clusters(&mut fs);
        // A fresh mount sees only what is on the disk.
        let mut disk = MemDisk::new(0);
        disk.sectors = fs.cache.device().sectors.clone();
        let mut again = Filesystem::mount(disk).unwrap();
        assert_eq!(read_file(&mut again, b"/f"), b"short");
        assert_eq!(again.next_cluster(first), None, "chain ends on disk");
        assert_eq!(free_clusters(&mut again), before);
    }

    #[test]
    fn a_failed_fat_copy_is_written_on_retry() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", &pattern(3000));
        let h = fs.open(b"/f", false);
        fs.cache.device().fail_lba = Some(fs.layout.fat_start + fs.layout.fat_size);
        assert!(!fs.write(h, b"short"));
        fs.cache.device().fail_lba = None;
        assert!(fs.write(h, b"short"));
        let (start, size) = (fs.layout.fat_start as usize, fs.layout.fat_size as usize);
        let disk = &fs.cache.device().sectors;
        assert_eq!(
            disk[start..start + size],
            disk[start + size..start + 2 * size]
        );
    }

    #[test]
    fn a_failed_empty_write_keeps_the_file() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", &pattern(1500));
        let h = fs.open(b"/f", false);
        fs.cache.device().fail_lba = Some(fs.layout.cluster_lba(fs.layout.root_cluster));
        let before = free_clusters(&mut fs);
        assert!(!fs.write(h, b""));
        fs.cache.device().fail_lba = None;
        assert_eq!(free_clusters(&mut fs), before, "chain still in use");
        assert_eq!(read_file(&mut fs, b"/f"), pattern(1500));
    }

    #[test]
    fn removed_files_invalidate_their_handles() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", b"data");
        let h = fs.open(b"/f", false);
        assert!(fs.remove(b"/f"));
        assert!(!fs.write(h, b"stale"));
        assert_eq!(fs.read(h, 0, &mut [0; 4]), None);
        write_file(&mut fs, b"/g", b"other");
        assert_eq!(read_file(&mut fs, b"/g"), b"other");
    }

    #[test]
    fn dot_and_dot_dot() {
        let mut fs = fs(1);
        assert!(fs.mkdir(b"/a"));
        assert!(fs.mkdir(b"/a/b"));
        write_file(&mut fs, b"/a/f", b"in a");
        assert_eq!(read_file(&mut fs, b"/a/b/../f"), b"in a");
        assert_eq!(read_file(&mut fs, b"/a/./b/./../f"), b"in a");
        assert_eq!(fs.stat(b"/a/.."), Stat::Directory);
        assert_eq!(names(&mut fs, b"/a/b/../.."), [b"a".to_vec()]);
        assert_eq!(fs.stat(b"/.."), Stat::Directory);
    }

    #[test]
    fn files_are_not_directories() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", &pattern(600));
        assert_eq!(fs.stat(b"/f/x"), Stat::Missing);
        assert!(!fs.mkdir(b"/f/x"));
        assert_eq!(fs.list(b"/f", 0, &mut [0; NAME_MAX]), None);
        assert_eq!(fs.stat(b"/f/"), Stat::Missing);
        assert_eq!(fs.stat(b"/f/."), Stat::Missing);
        assert_eq!(fs.open(b"/f/.", false), 0);
        assert_eq!(fs.stat(b"/f"), Stat::File);
    }

    #[test]
    fn directories_grow_past_one_cluster() {
        let mut fs = fs(1);
        // 16 entries per 512-byte cluster; each name below takes 2.
        let names_in: Vec<Vec<u8>> = (0..40)
            .map(|i| alloc::format!("file{i:02}").into_bytes())
            .collect();
        for name in &names_in {
            let mut path = b"/".to_vec();
            path.extend_from_slice(name);
            write_file(&mut fs, &path, name);
        }
        assert_eq!(names(&mut fs, b"/"), names_in);
        assert_eq!(read_file(&mut fs, b"/file39"), b"file39");
    }

    #[test]
    fn long_names_across_a_cluster_boundary() {
        let mut fs = fs(1);
        // Fill 15 of the root's 16 slots, then add a 3-entry name.
        for i in 0..15 {
            write_file(&mut fs, alloc::format!("/F{i}").as_bytes(), b"");
        }
        let long = b"/a-name-of-over-26-chars.txt";
        write_file(&mut fs, long, b"spanning");
        assert_eq!(read_file(&mut fs, long), b"spanning");
        assert!(names(&mut fs, b"/").iter().any(|n| n == &long[1..]));
        assert!(fs.remove(long));
        assert!(!names(&mut fs, b"/").iter().any(|n| n == &long[1..]));
        assert_eq!(fs.stat(long), Stat::Missing);
    }

    #[test]
    fn short_names_with_case_flags() {
        // As written by mtools for a name that fits 8.3 in lower case.
        let mut fs = fs(1);
        let entry = Entry {
            name: *b"README  TXT",
            attr: dir::ATTR_ARCHIVE,
            case: 0x18,
            ..Entry::default()
        };
        let root = fs.layout.cluster_lba(2);
        fs.cache.update(root, |s| entry.store(&mut s[..ENTRY_SIZE]));
        assert_eq!(names(&mut fs, b"/"), [b"readme.txt".to_vec()]);
        assert_eq!(fs.stat(b"/readme.txt"), Stat::File);
        assert_eq!(
            fs.stat(b"/README.TXT"),
            Stat::Missing,
            "names are case-sensitive"
        );
    }

    #[test]
    fn both_fats_stay_identical() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/a", &pattern(4000));
        assert!(fs.mkdir(b"/d"));
        write_file(&mut fs, b"/a", b"x");
        assert!(fs.remove(b"/d"));
        let (start, size) = (fs.layout.fat_start as usize, fs.layout.fat_size as usize);
        let disk = &fs.cache.device().sectors;
        assert_eq!(
            disk[start..start + size],
            disk[start + size..start + 2 * size]
        );
    }

    #[test]
    fn corrupt_chain_loop_terminates() {
        let mut fs = fs(1);
        write_file(&mut fs, b"/f", &pattern(2000));
        let h = fs.open(b"/f", false);
        let first = fs.handles[h as usize - 1].as_ref().unwrap().cluster;
        // Point the second cluster back at the first.
        let second = fs.fat_get(first);
        fs.fat_set(second, first);
        let mut buf = alloc::vec![0; 2000];
        assert!(fs.read(h, 0, &mut buf).is_some());
        // Overwriting must not free the clusters the loop points back into.
        assert!(fs.write(h, &pattern(1024)));
        assert_ne!(fs.fat_get(first), FAT_FREE);
        assert_ne!(fs.fat_get(second), FAT_FREE);
        assert_eq!(read_file(&mut fs, b"/f"), pattern(1024));
        // Nor overwrite its own first clusters when the data is longer.
        fs.fat_set(second, first);
        assert!(fs.write(h, &pattern(1536)));
        assert_eq!(read_file(&mut fs, b"/f"), pattern(1536));
        assert!(fs.remove(b"/f"));
    }

    #[test]
    fn full_disk_fails_cleanly() {
        let mut fs = fs(64);
        let free = free_clusters(&mut fs);
        let too_big = alloc::vec![1u8; (free + 1) * 64 * SECTOR_SIZE];
        let h = fs.open(b"/huge", true);
        assert!(!fs.write(h, &too_big));
        assert_eq!(free_clusters(&mut fs), free, "nothing leaked");
        assert!(fs.mkdir(b"/still-works"));
    }
}
