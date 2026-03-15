# Roadmap

## Current State

CassiOS boots via GRUB (multiboot), runs in 32-bit protected mode with a GDT (kernel + user segments + TSS), IDT, interrupt-driven PS/2 keyboard and mouse drivers, a VGA text-mode terminal, a shell with 22 commands, serial output (COM1), an in-kernel test framework, physical/heap memory management with paging, an in-memory filesystem, an ATA PIO block device driver, a syscall interface via int 0x80, preemptive round-robin scheduling, per-process address spaces, an ELF loader, and a ring 3 init process running alongside the kernel shell.

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

**Status**: Complete

Implement `int 0x80` dispatch with a syscall table. Start with a few trivial syscalls (e.g., `write` to the terminal) while still in a flat kernel. This separates the syscall plumbing from the complexity of ring 3 transitions and scheduling. Design decisions here (calling convention, table layout) affect everything after it.

Design: `docs/plans/2026-03-15-syscall-interface-design.md`

## Phase 7: Userspace and Process Management

**Status**: Complete

Add ring 3 execution, an ELF loader, preemptive scheduling, and per-process address spaces. A single init process loaded from a GRUB multiboot module runs alongside the existing kernel shell.

1. **GDT + TSS** — user code/data segments, Task State Segment for ring 0 stack on privilege transitions
2. **Process management** — Process struct, fixed-size process table (16 slots), ProcessManager singleton
3. **Scheduler** — preemptive round-robin via PIT timer, context switching by swapping saved ESP, single kernel stack with state saved to PCB
4. **Per-process address spaces** — separate page directories with shared kernel mappings, user pages in lower 3 GiB
5. **ELF loader** — parse ELF32 from multiboot modules, load PT_LOAD segments into user address spaces
6. **Ring 3 entry** — fake interrupt frame for initial iret to userspace, TSS.esp0 updates on context switch
7. **Init process** — separate userspace binary (`userspace/init/`), prints uptime every 5 seconds via syscalls

Design: `docs/plans/2026-03-16-userspace-process-management-design.md`

## Phase 8: IPC and Microkernel Transition

**Planning**: Brainstorm + design doc

Migrate from monolithic to microkernel architecture. The kernel shrinks to: IPC, scheduling, memory management, and interrupt routing. Everything else moves to userspace services.

1. **Interrupt subsystem refactor** — split InterruptManager into IDT owner, ExceptionHandler, IrqManager, and SyscallHandler (deferred from Phase 7 — justified now by IRQ forwarding and growing vector count)
2. **IPC mechanism** — synchronous message passing (send/receive/reply). This is the critical path for performance.
3. **IRQ forwarding** — kernel routes hardware interrupts to registered userspace driver processes via IPC
4. **Service migration** — move drivers (keyboard, mouse), filesystem, and shell out of the kernel into separate userspace processes
5. **Nameserver** — simple service for processes to find each other by name (e.g., shell looks up "vfs" to find the filesystem service)

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
