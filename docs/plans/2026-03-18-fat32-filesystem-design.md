# Phase 9: FAT32 Filesystem Design

## Overview

Replace the in-memory VFS with a real FAT32 filesystem backed by the ATA PIO block device. The VFS service owns FAT32 parsing directly and talks to ATA via IPC. The shell and VFS client API remain unchanged -- the swap is entirely internal to the VFS service.

## Key Decisions

- FAT32 parsing lives inside the VFS service (no separate FAT32 process)
- Build-time FAT32 image (32 MB, configurable) with pre-seeded files
- Long filename (LFN) support
- ATA IPC upgraded to full 512-byte sector transfers (replacing the old 16-byte/8-byte test scaffolding)
- Entire FAT table cached in RAM at mount time
- Same VFS IPC interface (message types 10-15) -- clients unchanged

## 1. Architecture and Data Flow

```
Shell --IPC--> VFS --IPC--> ATA --PIO--> Disk
               (FAT32)     (sectors)
```

### Boot Sequence

1. Nameserver, ATA, VGA, keyboard, mouse start as usual
2. VFS starts, looks up ATA via nameserver
3. VFS reads sector 0 from ATA, parses the BPB (BIOS Parameter Block)
4. VFS allocates a heap buffer and loads the entire FAT table from disk
5. VFS is ready to serve filesystem requests
6. Shell starts, uses VFS exactly as before

### Request Flow (read)

1. Shell sends `VfsOpen` with path to VFS
2. VFS resolves the path by reading directory sectors from ATA, matching entries
3. VFS returns a handle
4. Shell sends `VfsRead` with handle, offset, length
5. VFS walks the FAT chain (in-memory) to locate the correct clusters, issues multiple `AtaRead` IPC calls as needed, assembles the data
6. VFS replies with the full result via IPC data buffer

### Request Flow (write)

1. Shell sends `VfsOpen`, then `VfsWrite` with handle + data
2. VFS allocates clusters in the in-memory FAT as needed, writes data sectors to ATA
3. VFS writes the updated FAT sectors back to both on-disk copies
4. VFS updates the directory entry (file size) on disk

## 2. ATA IPC Upgrade

Replace the existing `AtaRead`/`AtaWrite` handlers with sector-granularity transfers using IPC data buffers. The old arg-packing logic (16 bytes read, 8 bytes write) is removed entirely.

### Message Semantics

**AtaRead (17)** -- read one 512-byte sector:
- Request: `arg1` = LBA
- Reply: `arg1` = 0 (success) or 1 (error), data buffer = 512 bytes

**AtaWrite (18)** -- write one 512-byte sector:
- Request: `arg1` = LBA, data buffer = 512 bytes
- Reply: `arg1` = 0 (success) or 1 (error)

### ATA Service Changes

- Receive loop uses `IPC::receive(&msg, dataBuf, 512)` to accept incoming data
- `AtaRead`: read sector via PIO, reply with `IPC::reply(sender, &reply, sectorBuf, 512)`
- `AtaWrite`: write the received buffer to disk, reply with status
- Single-sector cache inside `Ata` class remains (avoids redundant PIO)

### Client Helpers (libcassio)

```cpp
Ata::readSector(pid, lba, buf)    // sends AtaRead, receives 512 bytes
Ata::writeSector(pid, lba, buf)   // sends AtaWrite with 512 bytes
```

## 3. FAT32 On-Disk Structures

All structures defined by the FAT32 specification.

### Boot Sector / BPB (Sector 0)

Key fields parsed at mount time:
- `bytesPerSector` (512)
- `sectorsPerCluster` (typically 8 for 32 MB = 4 KiB clusters)
- `reservedSectors` (typically 32)
- `numFats` (2)
- `totalSectors`
- `fatSize32` (sectors per FAT)
- `rootCluster` (starting cluster of root directory, typically 2)

Derived values:
- `fatStartSector = reservedSectors`
- `dataStartSector = reservedSectors + (numFats * fatSize32)`
- `clusterToSector(n) = dataStartSector + (n - 2) * sectorsPerCluster`

### FAT Table

Array of 32-bit entries, one per cluster:
- `0x00000000` -- free cluster
- `0x00000002`..`0x0FFFFFEF` -- next cluster in chain
- `0x0FFFFFF8`..`0x0FFFFFFF` -- end of chain
- `0x0FFFFFF7` -- bad cluster

Loaded entirely into a heap-allocated buffer at mount time. Writes flush both on-disk copies.

### Short Directory Entry (32 bytes)

- `name[11]` -- 8.3 format (space-padded, uppercase)
- `attr` -- READ_ONLY, HIDDEN, SYSTEM, VOLUME_ID, DIRECTORY, ARCHIVE
- `createTime`, `createDate`, `modifyTime`, `modifyDate` -- written as zeros for now
- `firstClusterHigh` + `firstClusterLow` -- starting cluster
- `fileSize` -- 32-bit byte count (0 for directories)

### Long Filename Entry (32 bytes)

- `order` -- sequence number (bit 6 set on last entry)
- `name1[5]` -- first 5 UCS-2 characters
- `attr` = `0x0F` (LFN marker)
- `name2[6]` -- next 6 characters
- `name3[2]` -- final 2 characters
- `checksum` -- from associated 8.3 name

LFN entries stored in reverse order before the short entry. Each holds 13 characters.

## 4. VFS Internal Design

### Fat32 Class (replaces Filesystem)

**Member data:**
- `ataPid` -- ATA service PID
- `bpb` -- parsed boot sector parameters
- `fat` -- heap-allocated `u32` array (full FAT table)
- `fatSectors` -- FAT size for writeback

**Disk I/O:**
- `readSector(lba, buf)` / `writeSector(lba, buf)` -- IPC wrappers to ATA
- `clusterToLba(cluster)` -- cluster number to disk sector
- `readCluster(cluster, buf)` -- read all sectors of a cluster
- `writeCluster(cluster, buf)` -- write buffer to a cluster

**FAT management:**
- `allocateCluster()` -- scan FAT for free entry, mark as end-of-chain
- `freeChain(startCluster)` -- walk chain, mark all entries free
- `flushFat()` -- write in-memory FAT to both on-disk copies

**Directory operations:**
- `findEntry(dirCluster, name)` -- walk directory cluster chain, parse short+LFN entries, match by name; returns entry and on-disk location
- `resolvePath(path)` -- split by `/`, walk from root cluster calling `findEntry` at each level
- `listDir(dirCluster, index)` -- iterate entries, skip LFN/volume-label, return Nth visible entry name
- `createEntry(dirCluster, name, attr)` -- find free slot or extend directory, write LFN + short entry
- `removeEntry(dirCluster, name)` -- mark entries as deleted (0xE5), free cluster chain

**File operations (handle-based):**
- `open(path)` -- resolve path, populate handle table slot
- `read(handle, offset, buf, len)` -- walk FAT chain to locate clusters, read data, copy requested range
- `write(handle, data, len)` -- allocate clusters as needed, write data, update directory entry size, flush FAT
- `close(handle)` -- release handle slot

### Handle Table

Array of 16 slots, each storing:
- `startCluster` -- first cluster of the file
- `size` -- file size in bytes
- `dirCluster` + `dirOffset` -- on-disk location of directory entry (for size updates on write)
- `inUse` flag

### VFS main.cpp

Minimal change: replace `Filesystem fs` with `Fat32 fs`. Same IPC receive loop, same message dispatch. Each handler calls `fs.open()`, `fs.read()`, etc.

## 5. Build System

### Disk Image Target

`make disk` creates `bin/disk.img`:
1. `dd if=/dev/zero of=bin/disk.img bs=1M count=$(DISK_SIZE_MB)`
2. `mkfs.fat -F 32 bin/disk.img`
3. Copy seed files from `disk/` using `mtools` (`mcopy`, `mmd`)

Configuration: `DISK_SIZE_MB ?= 32` at the top of the Makefile.

### Integration

- `make run` depends on `disk`
- `make test-userspace` creates a test-specific disk image
- QEMU invocation: `-drive file=bin/disk.img,format=raw,if=ide`

### Seed Files

A `disk/` directory at the repo root contains files to pre-populate the image (e.g., `README.TXT`, a `docs/` directory). Gives immediate content on first boot.

### Host Dependencies

- `dosfstools` (provides `mkfs.fat`)
- `mtools` (provides `mcopy`, `mmd`)

## 6. Migration Plan

### Removed

- `userspace/vfs/include/filesystem.hpp` -- in-memory Filesystem class
- `userspace/vfs/src/filesystem.cpp` -- in-memory implementation

### Unchanged

- `common/include/message.hpp` -- same VFS message types (10-15); ATA types (17-18) keep constants, new semantics
- `userspace/libs/libcassio/include/vfs.hpp` -- client helpers unchanged
- `userspace/shell/` -- no changes
- VFS `main.cpp` -- same structure, different backing class

### Updated

- `userspace/drivers/ata/src/main.cpp` -- AtaRead/AtaWrite rewritten for 512-byte sector transfers
- `userspace/libs/libcassio/` -- add `Ata::readSector()` / `Ata::writeSector()` helpers
- `Makefile` -- add `disk` target, update dependencies

### Added

- `userspace/vfs/include/fat32.hpp` -- Fat32 class declaration
- `userspace/vfs/src/fat32.cpp` -- Fat32 implementation
- `userspace/vfs/include/fat32_structs.hpp` -- on-disk structure definitions (BPB, directory entry, LFN entry)
- `disk/` -- seed files for the image

## 7. Testing

- **Unit tests**: BPB parsing, cluster math, FAT chain traversal, LFN encoding/decoding, 8.3 name generation
- **Integration tests via IPC**: mkdir, open, read, write, list, remove -- adapted from existing VFS tests for persistent storage
- **Manual boot test**: `make run`, verify `ls`, `cat README.TXT`, `mkdir`, `write`, reboot and confirm persistence
