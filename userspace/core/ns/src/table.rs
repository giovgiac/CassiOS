// table.rs -- nameserver lookup table
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::vec::Vec;

use cassio::ns::{Entry, MAX_NAME_LEN};

struct Record {
    name: [u8; MAX_NAME_LEN],
    len: usize,
    pid: u32,
}

impl Record {
    fn name(&self) -> &[u8] {
        &self.name[..self.len]
    }
}

/// Name-to-PID table. Entries are listed most recently registered first.
pub struct NsTable {
    records: Vec<Record>,
}

impl NsTable {
    pub fn new() -> Self {
        Self {
            records: Vec::new(),
        }
    }

    /// Register `name` (truncated to MAX_NAME_LEN bytes) for `pid`.
    /// Returns 1 on success, 0 if the name is already registered or memory
    /// is exhausted.
    pub fn register(&mut self, name: &[u8], pid: u32) -> u32 {
        let name = &name[..name.len().min(MAX_NAME_LEN)];
        if self.lookup(name) != 0 || self.records.try_reserve(1).is_err() {
            return 0;
        }
        let mut record = Record {
            name: [0; MAX_NAME_LEN],
            len: name.len(),
            pid,
        };
        record.name[..name.len()].copy_from_slice(name);
        self.records.push(record);
        1
    }

    /// Return the PID registered for `name`, or 0 if none.
    pub fn lookup(&self, name: &[u8]) -> u32 {
        self.records
            .iter()
            .find(|r| r.name() == name)
            .map_or(0, |r| r.pid)
    }

    /// Fill `buf` with entries, most recent first. Returns the count written.
    pub fn list_all(&self, buf: &mut [Entry]) -> usize {
        let mut count = 0;
        for (slot, record) in buf.iter_mut().zip(self.records.iter().rev()) {
            *slot = Entry::default();
            slot.name[..record.len].copy_from_slice(record.name());
            slot.pid = record.pid;
            count += 1;
        }
        count
    }

    #[cfg(test)]
    pub fn count(&self) -> usize {
        self.records.len()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn name(entry: &Entry) -> &[u8] {
        let len = entry.name.iter().position(|&b| b == 0).unwrap();
        &entry.name[..len]
    }

    #[test]
    fn register_and_lookup() {
        let mut table = NsTable::new();
        assert_eq!(table.register(b"test", 42), 1);
        assert_eq!(table.lookup(b"test"), 42);
    }

    #[test]
    fn lookup_unregistered() {
        let table = NsTable::new();
        assert_eq!(table.lookup(b"nope"), 0);
    }

    #[test]
    fn register_multiple() {
        let mut table = NsTable::new();
        table.register(b"a", 1);
        table.register(b"b", 2);
        table.register(b"c", 3);
        assert_eq!(table.lookup(b"a"), 1);
        assert_eq!(table.lookup(b"b"), 2);
        assert_eq!(table.lookup(b"c"), 3);
    }

    #[test]
    fn register_duplicate_fails() {
        let mut table = NsTable::new();
        assert_eq!(table.register(b"dup", 1), 1);
        assert_eq!(table.register(b"dup", 2), 0);
        assert_eq!(table.lookup(b"dup"), 1);
    }

    #[test]
    fn beyond_old_limit() {
        let mut table = NsTable::new();
        for i in 0..20u8 {
            let name = [b'a' + i / 26, b'a' + i % 26, b'x'];
            assert_eq!(table.register(&name, i as u32 + 1), 1);
        }
        assert_eq!(table.count(), 20);
    }

    #[test]
    fn long_name_truncated() {
        let mut table = NsTable::new();
        table.register(b"0123456789abcdefXYZ", 7);
        assert_eq!(table.lookup(b"0123456789abcdef"), 7);
    }

    #[test]
    fn list_all_returns_entries() {
        let mut table = NsTable::new();
        table.register(b"vga", 3);
        table.register(b"kbd", 2);

        let mut buf = [Entry::default(); 8];
        assert_eq!(table.list_all(&mut buf), 2);
        assert!(buf[..2].iter().any(|e| name(e) == b"vga" && e.pid == 3));
        assert!(buf[..2].iter().any(|e| name(e) == b"kbd" && e.pid == 2));
    }

    #[test]
    fn list_all_most_recent_first() {
        let mut table = NsTable::new();
        table.register(b"ns", 1);
        table.register(b"kbd", 2);
        table.register(b"vfs", 3);

        let mut buf = [Entry::default(); 8];
        table.list_all(&mut buf);
        assert_eq!(name(&buf[0]), b"vfs");
        assert_eq!(name(&buf[1]), b"kbd");
        assert_eq!(name(&buf[2]), b"ns");
    }

    #[test]
    fn list_all_empty() {
        let table = NsTable::new();
        let mut buf = [Entry::default(); 8];
        assert_eq!(table.list_all(&mut buf), 0);
    }

    #[test]
    fn list_all_respects_max() {
        let mut table = NsTable::new();
        table.register(b"a", 1);
        table.register(b"b", 2);
        table.register(b"c", 3);

        let mut buf = [Entry::default(); 2];
        assert_eq!(table.list_all(&mut buf), 2);
    }
}
