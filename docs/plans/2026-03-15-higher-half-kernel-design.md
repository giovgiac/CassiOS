# Higher-Half Kernel Design

## Overview

Remap the kernel to virtual address 0xC0000000+ (3 GiB), freeing the lower 3 GiB for future userspace. The kernel image is linked at VMA 0xC0100000 but loaded by GRUB at LMA 0x100000. A bootstrap page table bridges the gap during early boot.

## Constants

- `KERNEL_VBASE = 0xC0000000` -- defined in `include/memory/virtual.hpp` as a `#define` so it's usable from both C++ and assembly.
- Kernel VMA: `0xC0100000` (physical 0x100000 + offset)
- VGA virtual address: `0xC00B8000` (physical 0xB8000 + offset)

## Linker Script

Update `src/linker.ld`:
- Set VMA to `0xC0100000`
- Add `AT(addr - KERNEL_VBASE)` load-address directives so GRUB loads at physical 0x100000
- `.multiboot` section stays at the physical load address (must be in first 8 KiB for GRUB)
- `_kernel_start` and `_kernel_end` become virtual addresses

## Bootstrap Assembly

Static page tables in `loader.s` (or a new `boot.S` if cleaner). Boot sequence:

1. GRUB enters at physical 0x100000. Save EAX (magic) and EBX (multiboot pointer).
2. Set up bootstrap page directory and one page table (statically allocated, page-aligned):
   - PDE[0] -> page table -> identity-maps physical 0x000000-0x3FFFFF (first 4 MiB)
   - PDE[768] -> same page table -> maps virtual 0xC0000000-0xC03FFFFF to physical 0x000000-0x3FFFFF
3. Load CR3 with the physical address of the bootstrap page directory.
4. Enable paging (set CR0.PG).
5. Absolute jump to a high-address label (moves EIP from 0x100xxx to 0xC01xxxxx).
6. Invalidate the low mapping: zero PDE[0], reload CR3 to flush TLB.
7. Load ESP with the virtual stack top address.
8. Add KERNEL_VBASE to EBX (translate multiboot pointer to virtual).
9. Call `ctors()` then `start()` as before.

Both mappings are active between steps 4 and 6, allowing the jump from low to high addresses.

## PagingManager Changes

`PagingManager::init()` builds the real page tables and replaces the bootstrap tables:

- **Mapping strategy**: Direct-map all physical RAM at KERNEL_VBASE. Every physical address P maps to virtual P + 0xC0000000. `mapPage(phys, phys + KERNEL_VBASE, flags)`.
- **VGA**: Falls within the direct-map naturally (physical 0xB8000 -> virtual 0xC00B8000). Explicit VGA loop uses the offset.
- **CR3 load**: `pageDirectory` is a virtual address; CR3 needs physical. Use `(u32)pageDirectory - KERNEL_VBASE`.
- **Page table access**: `allocFrame()` returns physical addresses. Dereference via `phys + KERNEL_VBASE`. Same for reading existing PDE entries to access page tables.
- **No paging enable**: Bootstrap already enabled paging; `init()` just builds tables and switches CR3.

Specific changes in `mapPage()`:
- Allocated page table frames: dereference as `(u32*)((u32)frame + KERNEL_VBASE)`
- Reading existing page tables from PDEs: `(u32*)((pde & 0xFFFFF000) + KERNEL_VBASE)`
- Same pattern in `unmapPage()`

After loading the new CR3, the bootstrap page tables are abandoned (static memory, no freeing needed).

## Other Affected Components

**VgaTerminal** (`src/hardware/terminal.cpp`):
- Buffer pointer changes from `0xB8000` to `KERNEL_VBASE + 0xB8000`.

**PhysicalMemoryManager** (`src/memory/physical.cpp`):
- Multiboot pointer arrives already translated (loader adds offset).
- `_kernel_start`/`_kernel_end` are virtual; subtract KERNEL_VBASE when computing physical frame numbers for marking kernel frames as used.
- Bitmap is in kernel .bss (virtual address); subtract KERNEL_VBASE when marking bitmap frames as used.

**KernelHeap** (`src/memory/heap.cpp`):
- `allocFrame()` returns physical; add KERNEL_VBASE to get the heap base virtual pointer.

**GDT**: No changes. Flat model with zero base and 4 GiB limit works unchanged.

**I/O ports**: Unaffected. Port I/O uses `in`/`out` instructions, not memory-mapped addresses.

**Test files**:
- `tests/memory/test_paging.cpp`: update 0xB8000 and 0x100000 to virtual equivalents.
- `tests/hardware/test_terminal.cpp`: update VGA buffer address checks.

## Testing

**Automated (`make test`)**:
- Update paging tests to verify KERNEL_VBASE offset mappings.
- Update terminal tests for new VGA virtual address.
- Add test verifying kernel symbols are above 0xC0000000.

**Manual (QEMU screenshot)**:
- Shell prompt on screen = full chain works (bootstrap -> high jump -> PagingManager -> VGA -> heap -> drivers).

**Debugging**:
- Triple fault: `qemu-system-i386 -d int -D logfile` to capture the faulting address.
- Serial (COM1) uses port I/O, works regardless of page mappings -- use for diagnostics if VGA is broken.
- QEMU monitor `info registers` / `info mem` to inspect CR3 and active mappings.
