# Roadmap

## Current State

CassiOS boots via GRUB (multiboot), runs in 32-bit protected mode with flat segmentation (monolithic kernel, all ring 0). Has a GDT, IDT, interrupt-driven PS/2 keyboard and mouse drivers, a VGA text-mode terminal, a shell with 21 commands, serial output (COM1), an in-kernel test framework, physical/heap memory management with paging, an in-memory filesystem, and an ATA PIO block device driver for IDE disk access.

## Phase 1: Memory Management

**Status**: Complete

Add three layers of memory management:

1. **Physical memory manager** (#41) — bitmap allocator tracking 4 KiB frames, parses the multiboot memory map to discover available RAM, bumps GDT limits to 4 GiB
2. **Kernel heap** (#42) — fixed 1 MiB free list allocator (first-fit) with global `operator new`/`delete`, backed by physical frames
3. **Paging** (#43) — identity map (VA = PA) for all usable RAM, page directory + page tables, CR0.PG enabled

Design: `docs/plans/2026-03-10-memory-management-design.md`

## Phase 2: In-Memory Filesystem

**Status**: Complete

Simple custom filesystem in RAM to exercise the memory management layer. Flat directory structure, files stored as page chains, basic operations: create, read, write, delete, list. Not FAT32 — just a minimal design to get file operations working.

## Phase 3: PIT Timer

**Status**: Complete

Program the Programmable Interval Timer (channel 0, IRQ 0) for periodic ticks. Provides a system tick counter and a `sleep()` primitive. Needed later for preemptive scheduling, but useful immediately for timing and debugging.

## Phase 4: ATA PIO Driver

**Status**: Complete

ATA PIO block device driver — port I/O to read/write 512-byte sectors from an IDE disk. Test with raw sector read/write shell commands. No filesystem layer yet — just the block device interface.

## Phase 5: Higher-Half Kernel

**Status**: Complete

Remap the kernel to 0xC0000000+ (upper 1 GiB). The lower 3 GiB becomes available for userspace. Requires updating the bootloader handoff, page tables, GDT, and all hardcoded addresses. Prerequisite for user/kernel address space separation. Small in code but high risk of subtle bugs (triple faults from mapping errors) — needs upfront planning.

Design: `docs/plans/2026-03-15-higher-half-kernel-design.md`

## Phase 6: Syscall Interface

**Planning**: Brainstorm + design doc

Implement `int 0x80` dispatch with a syscall table. Start with a few trivial syscalls (e.g., `write` to the terminal) while still in a flat kernel. This separates the syscall plumbing from the complexity of ring 3 transitions and scheduling. Design decisions here (calling convention, table layout) affect everything after it.

## Phase 7: Userspace and Process Management

**Planning**: Brainstorm + design doc

1. **Ring 3 execution** — TSS setup, user-mode code/data segments in the GDT, transition to ring 3
2. **ELF loader** — parse and load ELF binaries into per-process address spaces
3. **Scheduler** — preemptive round-robin using the PIT timer, context switching (save/restore registers + page directory)
4. **Per-process address spaces** — separate page directories, copy-on-write or simple cloning for fork-like semantics

## Phase 8: IPC and Microkernel Transition

**Planning**: Brainstorm + design doc

Migrate from monolithic to microkernel architecture. The kernel shrinks to: IPC, scheduling, memory management, and interrupt routing. Everything else moves to userspace services.

1. **IPC mechanism** — synchronous message passing (send/receive/reply). This is the critical path for performance.
2. **IRQ forwarding** — kernel routes hardware interrupts to registered userspace driver processes via IPC
3. **Service migration** — move drivers (keyboard, mouse), filesystem, and shell out of the kernel into separate userspace processes
4. **Nameserver** — simple service for processes to find each other by name (e.g., shell looks up "vfs" to find the filesystem service)

## Phase 9: FAT32 Filesystem

**Planning**: Brainstorm + design doc

FAT32 filesystem as a userspace service on top of the ATA PIO block device. Directories, files, and cluster-chain allocation. Implemented as a userspace process communicating via IPC from the start. Replaces the in-memory filesystem — needs a migration plan for the existing shell commands and VFS interface.

## Phase 10: VGA Graphics Mode

**Planning**: Brainstorm + design doc

Switch from VGA text mode to VGA graphics mode (mode 13h, 320x200, 256 colors) as a userspace driver. Framebuffer at 0xA0000, bitmap font rendering, 256-color palette. Implemented as a userspace display server communicating via IPC. Replaces the current VgaTerminal — needs a design for the transition and how the Terminal interface adapts.

## Future Considerations

- **Networking**: NE2000 or virtio-net driver, TCP/IP stack as a userspace service
- **Disk caching**: page cache layer between filesystem and block device
- **ACPI**: proper shutdown, hardware enumeration
