#include <memory/paging.hpp>
#include <memory/physical.hpp>
#include <memory/virtual.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::memory;

TEST(paging_read_write_after_enable) {
    // After paging is enabled with higher-half mapping, normal memory access should work.
    volatile u32 value = 0xDEADBEEF;
    ASSERT_EQ(value, 0xDEADBEEF);
    value = 0xCAFEBABE;
    ASSERT_EQ(value, 0xCAFEBABE);
}

TEST(paging_vga_memory_accessible) {
    // VGA memory should be mapped at KERNEL_VBASE + 0xB8000.
    volatile u16* vga = (volatile u16*)(KERNEL_VBASE + 0xB8000);
    u16 original = vga[0];
    vga[0] = 0x0741; // 'A' with light grey on black
    ASSERT_EQ(vga[0], 0x0741);
    vga[0] = original;
}

TEST(paging_kernel_memory_accessible) {
    // Kernel code at virtual 0xC0100000 should be accessible.
    volatile u8* kernel = (volatile u8*)(KERNEL_VBASE + 0x100000);
    // Just verify we can read from the kernel region without faulting.
    u8 byte = *kernel;
    (void)byte;
    ASSERT(true);
}

TEST(paging_heap_allocation_works) {
    // Heap should still work after paging is enabled.
    int* p = new int;
    ASSERT(p != nullptr);
    *p = 42;
    ASSERT_EQ(*p, 42);
    delete p;
}

TEST(paging_map_unmap_page) {
    PagingManager& pm = PagingManager::getManager();
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();

    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);

    // Map the physical frame to its direct-map virtual address and verify access.
    u32 phys = (u32)frame;
    u32 virt = phys + KERNEL_VBASE;
    pm.mapPage(virt, phys, PAGE_PRESENT | PAGE_READWRITE);
    pm.flushTLB(virt);

    volatile u32* ptr = (volatile u32*)virt;
    *ptr = 0x12345678;
    ASSERT_EQ(*ptr, 0x12345678);

    pmm.freeFrame(frame);
}

extern "C" u32 _kernel_start;

TEST(paging_kernel_symbols_above_vbase) {
    // Kernel symbols should be linked above KERNEL_VBASE.
    u32 addr = (u32)&_kernel_start;
    ASSERT(addr >= KERNEL_VBASE);
}
