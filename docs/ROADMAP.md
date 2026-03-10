# Roadmap

## Current State

CassiOS boots via GRUB (multiboot), runs in 32-bit protected mode with flat segmentation, has a GDT, IDT, interrupt-driven PS/2 keyboard and mouse drivers, a VGA text-mode terminal, a basic shell, serial output (COM1), and an in-kernel test framework. All memory is statically allocated or on the stack — no dynamic allocation exists.

## Phase 1: Memory Management

**Status**: In progress (issues #41-#43)

Add three layers of memory management:

1. **Physical memory manager** (#41) — bitmap allocator tracking 4 KiB frames, parses the multiboot memory map to discover available RAM, bumps GDT limits to 4 GiB
2. **Kernel heap** (#42) — fixed 1 MiB free list allocator (first-fit) with global `operator new`/`delete`, backed by physical frames
3. **Paging** (#43) — identity map (VA = PA) for all usable RAM, page directory + page tables, CR0.PG enabled

Design: `docs/plans/2026-03-10-memory-management-design.md`

## Phase 2: In-Memory Filesystem

Simple custom filesystem in RAM to exercise the memory management layer. Flat directory structure, files stored as page chains, basic operations: create, read, write, delete, list. Not FAT32 — just a minimal design to get file operations working.

## Phase 3: ATA PIO Driver + FAT32

1. **ATA PIO block device driver** — port I/O to read/write 512-byte sectors from an IDE disk
2. **FAT32 filesystem** — real-world filesystem format on top of the block device, supporting directories, files, and cluster-chain allocation

## Phase 4: VGA Graphics Mode

Switch from VGA text mode to VGA graphics mode (mode 13h, 320x200, 256 colors) by programming VGA registers directly from protected mode. Framebuffer at 0xA0000. Render text using a custom bitmap font onto the pixel framebuffer — a DOS-style terminal experience with full control over fonts, colors (256-color palette), and the ability to mix text with simple graphics (borders, logos) if desired.

## Future Considerations

- **Process management**: scheduler, context switching, syscalls. Requires higher-half kernel for user/kernel address space separation.
- **Higher-half kernel**: remap kernel to 0xC0000000+. Prerequisite for process management and user space.
