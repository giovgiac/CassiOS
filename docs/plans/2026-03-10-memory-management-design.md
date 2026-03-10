# Memory Management Design

## Overview

Add memory management to CassiOS in three layers: a physical memory manager (bitmap allocator), a kernel heap (free list allocator), and paging (identity map). Implementation order: physical memory manager, heap, then paging.

## Prerequisite: GDT Segment Limits

Bump both code and data segment limits from 128 MiB to 4 GiB so the kernel can address all available RAM.

## Layer 1: Physical Memory Manager

**Class**: `PhysicalMemoryManager` (singleton, `include/memory/physical.hpp`, `src/memory/physical.cpp`)

Tracks which 4 KiB physical frames are free or in use via a bitmap.

### Data structures

- Static bitmap in BSS: `u8 bitmap[131072]` (128 KiB, 1 bit per 4 KiB frame, covers 4 GiB)

### Interface

- `static PhysicalMemoryManager& getManager()` -- singleton access
- `void init(u32 multibootInfo)` -- parse multiboot memory map, initialize bitmap
- `void* allocFrame()` -- find first free bit, mark used, return physical address
- `void freeFrame(void* address)` -- clear the bit for the given frame
- `bool isFrameUsed(void* address)` -- query a frame's status

### Init sequence

1. Mark entire bitmap as used (all 1s)
2. Walk multiboot memory map, mark "available" regions as free (clear bits)
3. Re-mark the kernel's memory (linker symbols `_start` to `_end`) as used
4. Re-mark the bitmap's own memory as used

### Memory discovery

Parse the multiboot memory map passed by GRUB to discover usable RAM dynamically. This adapts to whatever machine or QEMU configuration boots the kernel.

## Layer 2: Kernel Heap

**Class**: `HeapAllocator` (singleton, `include/memory/heap.hpp`, `src/memory/heap.cpp`)

Fixed 1 MiB region for variable-sized kernel allocations using a first-fit free list.

### Data structures

Block header for each block (free or allocated):

```cpp
struct BlockHeader {
    u32 size;
    bool free;
    BlockHeader* next;
};
```

### Interface

- `static HeapAllocator& getAllocator()` -- singleton access
- `void init()` -- request 256 frames (1 MiB) from the physical memory manager, initialize as a single free block
- `void* allocate(usize size)` -- first-fit search, split if remainder is large enough
- `void free(void* ptr)` -- mark block free, coalesce with adjacent free neighbors

### Global operators (`src/memory/operators.cpp`)

- `void* operator new(usize size)` -- calls `HeapAllocator::getAllocator().allocate(size)`
- `void* operator new[](usize size)` -- same
- `void operator delete(void* ptr)` -- calls `HeapAllocator::getAllocator().free(ptr)`
- `void operator delete[](void* ptr)` -- same
- `void operator delete(void* ptr, usize)` -- sized delete overload

### Notes

- The heap is for small, variable-sized kernel objects. Large allocations (e.g., filesystem file contents) should allocate whole pages directly from the physical memory manager.
- The 1 MiB size is fixed and does not grow.

## Layer 3: Paging

**Class**: `PagingManager` (singleton, `include/memory/paging.hpp`, `src/memory/paging.cpp`)

Identity-maps all usable RAM (VA = PA) and enables paging via CR0.

### Data structures

- Page directory: 1024 entries (4 KiB), statically allocated
- Page tables: allocated from the physical memory manager during init

### Interface

- `static PagingManager& getManager()` -- singleton access
- `void init()` -- build identity map, enable paging
- `void mapPage(u32 virtualAddr, u32 physicalAddr, u16 flags)` -- map a single 4 KiB page
- `void unmapPage(u32 virtualAddr)` -- unmap a single page
- `void flushTLB(u32 virtualAddr)` -- `invlpg` for a single address

### Init sequence

1. Zero the page directory
2. Identity-map all usable memory reported by the physical memory manager, plus memory-mapped I/O regions (VGA at 0xB8000, etc.)
3. Allocate page tables from the physical memory manager as needed
4. Load page directory into CR3
5. Set PG bit in CR0

## Boot Integration

In `kernel.cpp` `start()`, after existing initialization (GDT, InterruptManager, DriverManager):

```
PhysicalMemoryManager::getManager().init(multibootInfo);
HeapAllocator::getAllocator().init();
PagingManager::getManager().init();
```

## Testing

All three layers get unit tests via the in-kernel test framework:

- **Physical memory manager**: alloc/free frames, double-free detection, bitmap consistency, alloc until exhaustion
- **Heap**: allocate/free objects of various sizes, coalescing, split behavior, operator new/delete
- **Paging**: after enabling paging, verify memory reads/writes still work (identity map correctness), map/unmap individual pages

## Implementation Plan

Three separate GitHub issues and PRs, implemented in order:

1. Physical memory manager (includes GDT limit bump)
2. Kernel heap with operator new/delete
3. Paging with identity map
