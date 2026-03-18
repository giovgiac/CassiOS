# Full Codebase Review -- 2026-03-18

Comprehensive review of the CassiOS codebase after Phase 9 (FAT32 Filesystem) completion. Covers kernel, userspace services, build system, and CLAUDE.md compliance.

## Critical / High Severity

### 1. Non-reentrant shared interrupt variables in stub.s

`kernel/src/hardware/stub.s`, lines 94-95

The `number` and `error_code` variables are single `.data` locations shared by all interrupt and exception stubs. A nested IRQ (e.g., IRQ 0 fires while IRQ 1 is being dispatched) overwrites the slot before the outer handler reads it. The same slot is shared between interrupt and exception paths, so an exception during IRQ dispatch corrupts both.

### 2. GDT and TSS are stack-allocated; Scheduler holds a raw pointer

`kernel/src/core/kernel.cpp`, line 32; `kernel/src/core/gdt.cpp`, line 24

`GlobalDescriptorTable gdt` is a local variable in `start()`. The GDTR and TSS point to stack addresses. `Scheduler` stores `&gdt` and calls `gdt->setTssEsp0()` on every context switch. This works only because `start()` never returns (it `hlt`-loops), but any accidental `ret` or stack corruption would make the GDTR and TSS dangle. Architecturally fragile.

### 3. sbrk does not TLB-flush new user pages

`kernel/src/core/syscall.cpp`, lines 370-377

`sbrk` calls `paging.mapUserPage()` which writes page tables but does not issue `invlpg` for the newly mapped pages. After returning to userspace, the process may fault on first access to freshly-sbrked pages due to stale TLB entries.

### 4. ELF loader: p_offset + segOffset can wrap (u32)

`kernel/src/core/elf.cpp`, line 92

`ph->p_offset + segOffset` are both `u32`. A malformed ELF with `p_offset` near `0xFFFFFFFF` wraps the addition to a small number, passes the `< elfSize` check, and reads from the beginning of `elfData` -- potentially disclosing kernel-mapped memory into a userspace page. `p_offset` is never validated to be `<= elfSize` before the loop.

### 5. mapDevice syscall is unprivileged

`kernel/src/core/syscall.cpp`, lines 297-315

Any userspace process can call `mapDevice` with an arbitrary physical address and map it into its own address space with `PAGE_USER`. No check that the physical range is legitimate device MMIO. Combined with IOPL=3 in initial EFLAGS (kernel.cpp line 98), this gives all userspace processes full hardware access.

### 6. FAT32 removeEntry: cross-cluster LFN broken

`userspace/vfs/src/fat32/filesystem.cpp`, lines 773-774, 837-840

`lfnStart` and `inLfn` are declared inside the per-cluster loop, resetting on every iteration. When an LFN sequence starts at the end of cluster N and the short entry is at the start of cluster N+1: (a) the wrong name is extracted (shortNameToString instead of LFN), so name matching fails and the entry is never deleted; (b) even if matched, the LFN entries in the previous cluster are never marked `DIR_ENTRY_FREE`. The same `lfnBuf` fix applied to `readDirEntry` is needed here.

### 7. FAT32 write(0 bytes) leaks the first cluster

`userspace/vfs/src/fat32/filesystem.cpp`, lines 1041, 1079-1085

When `len == 0` and the file already has clusters: the write loop is skipped, the truncation block marks the first cluster as `FAT_EOC` but does not free it. The directory entry retains `firstCluster` pointing to the orphaned cluster. One cluster is permanently leaked per zero-length write to an existing file.

## Medium Severity

### 8. FAT32 createEntry: missing error check on readCluster

`userspace/vfs/src/fat32/filesystem.cpp`, line 698

When LFN entries span a cluster boundary, `readCluster(writeCluster_, clusterBuf)` can fail (I/O error) but the return value is not checked. Subsequent writes corrupt whatever garbage was in `clusterBuf`.

### 9. FAT32 open(): handle table full leaks directory entry

`userspace/vfs/src/fat32/filesystem.cpp`, lines 948-980

When `create=true` and `createEntry()` succeeds but no free handle slot exists, the function returns 0. The created directory entry (and for directories, an allocated content cluster) remain on disk permanently. Ghost entries appear in listings but can never be opened.

### 10. FAT32 nameToShort: always generates ~1 tail

`userspace/vfs/src/fat32/filesystem.cpp`, lines 256-260

Every file needing an 8.3 name gets `<PREFIX>~1.<EXT>`. Two files `longname_a.txt` and `longname_b.txt` both get `LONGNA~1.TXT`. This violates the FAT32 spec requirement for unique short names within a directory. External tools reading the volume will see corrupt entries.

### 11. Kernel destroy() does not wake SendBlocked senders

`kernel/src/core/process.cpp`, lines 149-181

`destroy` drains the send queue but never transitions waiting senders from `SendBlocked` to a runnable or error state. Processes that called `send()` to a destroyed target are permanently blocked with no way to be woken -- they become zombies.

### 12. Shell buffer overflow in execute()

`userspace/shell/src/shell.cpp`, line 98; `userspace/shell/include/shell.hpp`, line 27

`buffer` is `char[SHELL_MAX_INPUT]` (78 bytes, indices 0-77). Characters are inserted only when `length < SHELL_MAX_INPUT`, so `length` can reach 78. `execute()` writes `buffer[length] = '\0'` -- when `length == 78`, this is one byte past the array. The next field in memory is `length` (a `u8`), which gets overwritten with 0.

### 13. Heap: no double-free protection

`common/src/heap.cpp`, line 65

`free()` sets `block->free = true` unconditionally. A double-free silently corrupts the heap -- the subsequent coalescing walk can merge live blocks into the re-freed block.

### 14. Heap: no bounds check on free pointer

`common/src/heap.cpp`, line 60

The only guard is `if (!ptr) return`. Any non-null pointer outside the managed region causes `(u8*)ptr - sizeof(BlockHeader)` to be treated as a valid `BlockHeader`, corrupting arbitrary memory. No range check exists.

### 15. Heap extend() underflow

`common/src/heap.cpp`, line 97

`newBlock->size = additionalSize - sizeof(BlockHeader)` -- both are unsigned. If `additionalSize < sizeof(BlockHeader)` (12 bytes), this wraps to a huge value. Unlikely via `UserHeap` (rounds up to 4096) but the function itself has no guard.

### 16. Build: $(DISK) never rebuilds when seed files change

`Makefile`, line 128

The `$(DISK)` target has no prerequisites. Once `bin/disk.img` exists, changing files under `disk/` has no effect until `rm bin/disk.img` is run manually. Should list seed files as prerequisites.

## Low Severity

### 17. FAT32 resolvePath with ..: entryCluster/entryOffset not updated

`userspace/vfs/src/fat32/filesystem.cpp`, lines 466-491

When `..` is processed, `entryCluster` and `entryOffset` retain values from the previous component. Not currently exploitable because `open()` rejects directories, but a latent correctness issue.

### 18. Vfs::list 64-byte buffer truncation

`userspace/libs/libcassio/include/vfs.hpp`, lines 87, 95-96

The buffer is the same size as `SHELL_MAX_PATH` (64). Paths of exactly 63 characters produce uninitialized bytes in the IPC payload because the buffer has no room for null termination at that length.

### 19. Vfs::read sends caller's buffer as IPC data

`userspace/libs/libcassio/include/vfs.hpp`, line 56

The read request sends `buf` (uninitialized caller memory) to the VFS service. The VFS ignores incoming data for reads, so this is harmless but wastes IPC bandwidth and leaks stack contents across address spaces.

### 20. Paging: mapPage() does not flush TLB after new PDE

`kernel/src/memory/paging.cpp`, lines 76-95

When a new page directory entry is created at runtime, no `invlpg` is issued. Stale TLB entries from a former PDE could cause incorrect mappings.

### 21. EFLAGS IOPL=3 gives all userspace processes direct I/O port access

`kernel/src/core/kernel.cpp`, line 98

Initial EFLAGS is `0x3202` (IOPL=3). This grants ring-3 processes direct I/O port access, bypassing the kernel's port abstraction. Intentional design choice, but a security boundary gap.

### 22. sbrk does not check for overflow or stack collision

`kernel/src/core/syscall.cpp`, lines 365-380

`oldBreak + increment` can wrap on u32. The mapped range could overlap the user stack at `0xBFFFF000`. The kernel should enforce a ceiling.

## CLAUDE.md Compliance

### 23. Missing deleted copy/move on hardware-bound classes

CLAUDE.md: "Copy/move constructors and assignment operators are explicitly deleted on hardware-bound and singleton classes"

The following hardware-bound classes have no deleted special member functions:
- `userspace/drivers/ata/include/ata.hpp` -- `Ata` (9 Port<> members)
- `userspace/drivers/vga/include/terminal.hpp` -- `VgaTerminal` (Port<> CRTC members)
- `userspace/drivers/kbd/include/keyboard.hpp` -- `Keyboard` (PS/2 hardware)
- `userspace/drivers/mouse/include/mouse.hpp` -- `Mouse` (PS/2 hardware)
- `userspace/vfs/include/fat32/filesystem.hpp` -- `Fat32Filesystem` (sector cache, disk I/O)
- `userspace/ns/include/table.hpp` -- `NsTable` (deletes copy but not move)

### 24. Fixed arrays for non-hardware-dictated sizes

CLAUDE.md: "Use the kernel heap for dynamic data structures rather than defaulting to fixed-size arrays. Fixed arrays are appropriate only when the size is hardware-dictated"

- `Fat32Filesystem::handles[MAX_HANDLES]` (MAX_HANDLES=16) -- software limit
- `Fat32Filesystem::cache[CACHE_SIZE]` (CACHE_SIZE=16) -- policy choice

### 25. VfsStat message type out of sequence

`common/include/message.hpp`, line 51

`VfsStat = 19` is placed after VFS types 10-15 but numbered after ATA types 17-18. Creates a confusing gap in the VFS message number range. `MouseRead = 16` occupies the slot that would logically follow the VFS group.

## Summary

| # | Severity | Area | Issue |
|---|----------|------|-------|
| 1 | Critical | Kernel (stub.s) | Shared interrupt/exception variables -- reentrancy corruption |
| 2 | High | Kernel (GDT) | Stack-allocated GDT/TSS; Scheduler holds raw pointer to stack |
| 3 | High | Kernel (sbrk) | No TLB flush for new user pages |
| 4 | High | Kernel (ELF) | p_offset wrap allows kernel memory disclosure |
| 5 | High | Kernel (syscall) | mapDevice unprivileged -- arbitrary physical memory mapping |
| 6 | High | FAT32 | removeEntry cross-cluster LFN broken |
| 7 | High | FAT32 | write(0 bytes) leaks first cluster |
| 8 | Medium | FAT32 | createEntry missing error check on readCluster |
| 9 | Medium | FAT32 | open() handle-table-full leaks directory entry |
| 10 | Medium | FAT32 | nameToShort always generates ~1 (duplicate short names) |
| 11 | Medium | Kernel | destroy() leaves senders permanently SendBlocked |
| 12 | Medium | Shell | buffer[length] = '\0' off-by-one overflow |
| 13 | Medium | Heap | No double-free protection |
| 14 | Medium | Heap | No bounds check on free pointer |
| 15 | Medium | Heap | extend() underflow when additionalSize < sizeof(BlockHeader) |
| 16 | Medium | Build | $(DISK) never rebuilds when seed files change |
| 17 | Low | FAT32 | resolvePath(..) doesn't update entryCluster/entryOffset |
| 18 | Low | libcassio | Vfs::list buffer truncation at 63 chars |
| 19 | Low | libcassio | Vfs::read sends uninitialized buffer to server |
| 20 | Low | Kernel | mapPage() no TLB flush after new PDE |
| 21 | Low | Kernel | IOPL=3 gives all userspace direct I/O access |
| 22 | Low | Kernel | sbrk no overflow/stack-collision check |
| 23 | CLAUDE.md | Userspace | Missing deleted copy/move on 6 hardware-bound classes |
| 24 | CLAUDE.md | FAT32 | Fixed arrays for non-hardware-dictated sizes |
| 25 | CLAUDE.md | Common | VfsStat message type out of sequence |
