# Roadmap

## Current State

CassiOS boots via GRUB (multiboot), runs in 32-bit protected mode with flat segmentation (monolithic kernel, all ring 0). Has a GDT, IDT, interrupt-driven PS/2 keyboard and mouse drivers, a VGA text-mode terminal, a shell with 17 commands, serial output (COM1), an in-kernel test framework, physical/heap memory management with paging, and an in-memory filesystem.

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

## Phase 3: ATA PIO Driver + FAT32

1. **ATA PIO block device driver** — port I/O to read/write 512-byte sectors from an IDE disk
2. **FAT32 filesystem** — real-world filesystem format on top of the block device, supporting directories, files, and cluster-chain allocation

## Phase 4: VGA Graphics Mode

Switch from VGA text mode to VGA graphics mode (mode 13h, 320x200, 256 colors) by programming VGA registers directly from protected mode. Framebuffer at 0xA0000. Render text using a custom bitmap font onto the pixel framebuffer — a DOS-style terminal experience with full control over fonts, colors (256-color palette), and the ability to mix text with simple graphics (borders, logos) if desired.

## Phase 5: Higher-Half Kernel

Remap the kernel to 0xC0000000+ (upper 1 GiB). The lower 3 GiB becomes available for userspace. Requires updating the bootloader handoff, page tables, GDT, and all hardcoded addresses. Prerequisite for user/kernel address space separation.

## Phase 6: Userspace and Process Management

1. **Ring 3 execution** — TSS setup, user-mode code/data segments in the GDT, syscall interface (int 0x80 or sysenter)
2. **ELF loader** — parse and load ELF binaries into per-process address spaces
3. **Scheduler** — preemptive round-robin using the PIT timer, context switching (save/restore registers + page directory)
4. **Per-process address spaces** — separate page directories, copy-on-write or simple cloning for fork-like semantics

## Phase 7: IPC and Microkernel Transition

Migrate from monolithic to microkernel architecture. The kernel shrinks to: IPC, scheduling, memory management, and interrupt routing. Everything else moves to userspace services.

1. **IPC mechanism** — synchronous message passing (send/receive/reply). This is the critical path for performance.
2. **IRQ forwarding** — kernel routes hardware interrupts to registered userspace driver processes via IPC
3. **Service migration** — move drivers (keyboard, mouse, VGA), filesystem, and shell out of the kernel into separate userspace processes
4. **Nameserver** — simple service for processes to find each other by name (e.g., shell looks up "vfs" to find the filesystem service)

## Future Considerations

- **Networking**: NE2000 or virtio-net driver, TCP/IP stack (as a userspace service once microkernel is in place)
- **Disk caching**: page cache layer between filesystem and block device
- **ACPI**: proper shutdown, hardware enumeration
