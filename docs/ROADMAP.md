# Roadmap

## Current State

CassiOS is a microkernel OS. The kernel provides IPC (synchronous message passing), scheduling, memory management, and interrupt routing. Everything else runs as userspace services: keyboard, mouse, VGA terminal, ATA disk, FAT32 filesystem, nameserver, and an interactive shell with 16 commands. Boots via GRUB (multiboot), 32-bit protected mode, preemptive round-robin scheduling, per-process address spaces, ELF loader, serial output (COM1), and both kernel and userspace test frameworks. The filesystem is persistent on a 32 MiB FAT32 disk image with long filename support.

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

**Status**: Complete

Migrated from monolithic to microkernel architecture. The kernel now contains only: GDT/TSS, interrupt subsystem, memory management, process management, scheduler, PIT timer, IPC, ELF loader, and serial. Everything else runs as userspace services communicating via IPC.

1. **Interrupt subsystem refactor** (#96) — split InterruptManager into IDT owner, ExceptionHandler, IrqManager, and SyscallHandler
2. **IPC mechanism** (#97) — synchronous message passing (send/receive/reply) with fire-and-forget notify
3. **IRQ forwarding** (#98) — kernel routes hardware interrupts to registered userspace driver processes via IPC
4. **Userspace test framework** (#99) — TEST() macro with auto-registration, serial output, QEMU exit
5. **Nameserver** (#100, #101) — service discovery by name, first userspace service
6. **Driver migration** — keyboard (#102), VGA terminal (#103), filesystem (#104), mouse (#105), ATA (#106) moved to userspace
7. **Shell migration** (#107) — shell as userspace process, dead kernel code removed, PIT moved to hardware/, drivers grouped under userspace/drivers/

Design: `docs/plans/2026-03-16-ipc-microkernel-transition-design.md`

## Phase 9: FAT32 Filesystem

**Status**: Complete

Replaced the in-memory filesystem with FAT32 backed by the ATA PIO block device. The VFS service owns FAT32 parsing directly and talks to ATA via IPC. The shell and VFS client API remained unchanged — the swap was entirely internal to the VFS service.

1. **ATA IPC upgrade** (#124) — replaced 16-byte/8-byte arg-packed transfers with full 512-byte sector transfers using IPC data buffers
2. **Build system** (#125) — `make disk` creates a 32 MiB FAT32-formatted disk image with pre-seeded files via `mkfs.fat` and `mtools`
3. **FAT32 implementation** (#126) — Fat32 class with mount, BPB parsing, on-demand FAT via LRU sector cache, directory operations with LFN support, cluster-chain file I/O, VfsStat for path validation, case-sensitive filenames

Design: `docs/plans/2026-03-18-fat32-filesystem-design.md`

## Phase 10: VESA Framebuffer and Graphics Terminal

**Status**: Complete

Switch from VGA text mode to a VESA/VBE framebuffer (32bpp, requested via multiboot header so GRUB handles mode setting and falls back to the closest available resolution). Replace the VGA text-mode terminal with a software-rendered graphical terminal built on a simple 2D graphics library. The VGA service is replaced by a new `display` service (`userspace/drivers/display/`, `std::display::Display` client).

1. **VESA framebuffer** (#183) — multiboot video mode request, framebuffer discovery from multiboot info (actual resolution, pitch, bpp), MapDevice to map it into the display service's address space
2. **Graphics library (`std::gfx`)** (#183) — PixelBuffer class with drawing primitives: drawPixel, fillRect, drawRect, drawChar (bitmap font), drawText, scroll (ring buffer), blit. Operates on caller-provided buffers
3. **Display service** (#183, `userspace/drivers/display/`) — replaces the VGA driver. Owns the framebuffer and back buffer, uses `std::gfx` internally to draw. Exposes IPC commands for drawing, blit, scroll, and flush (copy back buffer to framebuffer). Pure hardware abstraction — draws pixels, knows nothing about text or terminals
4. **Terminal service** (#183, `userspace/core/terminal/`) — connects to the display service, renders a character grid with bitmap font, cursor, and scrolling. The shell and apps talk to the terminal (same IPC protocol as the current VGA service), the terminal translates to draw calls
5. **Migration** (#183) — replaced `std::vga::Vga` client with `std::terminal::Terminal`, removed old VGA driver, updated all service and app references

Double buffering (draw to RAM, flush to framebuffer) prevents tearing. Separating display from terminal keeps the driver clean and avoids a refactor when non-terminal apps (editor, GUI) need direct screen access later.

## Future Considerations

- **Window manager**: composite multiple app buffers, window decorations, mouse-driven input routing — path from terminal OS to desktop GUI using the same gfx primitives
- **Text editor**: fullscreen editor app (nano-like), works in both text mode and graphics terminal
- **Networking**: NE2000 or virtio-net driver, TCP/IP stack as a userspace service
- **Disk caching**: page cache layer between filesystem and block device
- **ACPI**: proper shutdown, hardware enumeration
