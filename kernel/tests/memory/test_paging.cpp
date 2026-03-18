#include <memory/paging.hpp>
#include <memory/physical.hpp>
#include <memory/virtual.hpp>
#include <test.hpp>

using namespace cassio;
using namespace std;
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
extern "C" u32 _kernel_end;

TEST(paging_kernel_symbols_above_vbase) {
    // Kernel symbols should be linked above KERNEL_VBASE.
    u32 start = (u32)&_kernel_start;
    u32 end = (u32)&_kernel_end;
    ASSERT(start >= KERNEL_VBASE);
    ASSERT(end > start);
}

TEST(paging_cr3_holds_physical_address) {
    // CR3 should contain a physical address (below KERNEL_VBASE).
    u32 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    ASSERT(cr3 < KERNEL_VBASE);
    ASSERT(cr3 != 0);
}

TEST(paging_heap_pointers_above_vbase) {
    // Heap allocations should return virtual addresses in the direct map.
    int* p = new int;
    ASSERT(p != nullptr);
    ASSERT((u32)p >= KERNEL_VBASE);
    delete p;
}

TEST(paging_stack_above_vbase) {
    // The stack pointer should be in the higher half.
    u32 esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    ASSERT(esp >= KERNEL_VBASE);
}

TEST(paging_pmm_returns_physical_addresses) {
    // allocFrame() should return physical addresses (below KERNEL_VBASE).
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);
    ASSERT((u32)frame < KERNEL_VBASE);
    pmm.freeFrame(frame);
}

TEST(paging_create_address_space_returns_physical) {
    PagingManager& pm = PagingManager::getManager();
    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);
    ASSERT(pd < KERNEL_VBASE);

    pm.destroyAddressSpace(pd);
}

TEST(paging_create_address_space_shares_kernel_pdes) {
    PagingManager& pm = PagingManager::getManager();
    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);

    // Read current kernel CR3 to get the kernel page directory physical address.
    u32 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    u32* kernelPd = (u32*)(cr3 + KERNEL_VBASE);
    u32* newPd = (u32*)(pd + KERNEL_VBASE);

    // Kernel PDEs (768-1023) must be identical.
    for (u32 i = 768; i < 1024; i++) {
        ASSERT_EQ(newPd[i], kernelPd[i]);
    }

    // User PDEs (0-767) must be zeroed.
    for (u32 i = 0; i < 768; i++) {
        ASSERT_EQ(newPd[i], 0u);
    }

    pm.destroyAddressSpace(pd);
}

TEST(paging_map_user_page_creates_mapping) {
    PagingManager& pm = PagingManager::getManager();
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();

    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);

    void* frame = pmm.allocFrame();
    ASSERT(frame != nullptr);

    u32 userVirt = 0x00400000;
    pm.mapUserPage(pd, userVirt, (u32)frame, PAGE_PRESENT | PAGE_READWRITE | PAGE_USER);

    // Verify the PTE was created by reading the page table.
    u32* pdPtr = (u32*)(pd + KERNEL_VBASE);
    u32 pdIndex = userVirt >> 22;
    ASSERT(pdPtr[pdIndex] & PAGE_PRESENT);

    u32* pt = (u32*)((pdPtr[pdIndex] & 0xFFFFF000) + KERNEL_VBASE);
    u32 ptIndex = (userVirt >> 12) & 0x3FF;
    ASSERT(pt[ptIndex] & PAGE_PRESENT);
    ASSERT_EQ(pt[ptIndex] & 0xFFFFF000, (u32)frame);

    pm.destroyAddressSpace(pd);
}
