#include <std/test.hpp>

#include <core/process.hpp>
#include <core/syscall.hpp>
#include <memory/paging.hpp>
#include <memory/physical.hpp>
#include <memory/virtual.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;
using namespace cassio::memory;

TEST(sbrk_returns_old_break) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    // Create a process with a page directory and heap break.
    PagingManager& paging = PagingManager::getManager();
    u32 pd = paging.createAddressSpace();
    ASSERT(pd != 0);

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, pd);
    ASSERT(p != nullptr);
    p->heapBreak = 0x00500000;
    pm.setCurrent(p);

    u32 oldBreak = sh.sbrk(4096);
    ASSERT_EQ(oldBreak, 0x00500000u);
    ASSERT_EQ(p->heapBreak, 0x00501000u);

    paging.destroyAddressSpace(pd);
    pm.destroy(p->pid);
}

TEST(sbrk_zero_returns_current_break) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    PagingManager& paging = PagingManager::getManager();
    u32 pd = paging.createAddressSpace();
    ASSERT(pd != 0);

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, pd);
    ASSERT(p != nullptr);
    p->heapBreak = 0x00600000;
    pm.setCurrent(p);

    u32 result = sh.sbrk(0);
    ASSERT_EQ(result, 0x00600000u);
    ASSERT_EQ(p->heapBreak, 0x00600000u); // Unchanged.

    paging.destroyAddressSpace(pd);
    pm.destroy(p->pid);
}

TEST(sbrk_maps_multiple_pages) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    PagingManager& paging = PagingManager::getManager();
    u32 pd = paging.createAddressSpace();
    ASSERT(pd != 0);

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, pd);
    ASSERT(p != nullptr);
    p->heapBreak = 0x00700000;
    pm.setCurrent(p);

    // Request 3 pages worth of memory.
    u32 oldBreak = sh.sbrk(3 * 4096);
    ASSERT_EQ(oldBreak, 0x00700000u);
    ASSERT_EQ(p->heapBreak, 0x00703000u);

    paging.destroyAddressSpace(pd);
    pm.destroy(p->pid);
}

TEST(sbrk_no_heap_returns_zero) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);
    p->heapBreak = 0; // No heap initialized.
    pm.setCurrent(p);

    u32 result = sh.sbrk(4096);
    ASSERT_EQ(result, 0u);

    pm.destroy(p->pid);
}

TEST(sbrk_overflow_returns_zero) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);
    p->heapBreak = 0x80000000;
    pm.setCurrent(p);

    // Increment would wrap u32.
    u32 result = sh.sbrk(0x80000001);
    ASSERT_EQ(result, 0u);
    ASSERT_EQ(p->heapBreak, 0x80000000u); // Unchanged.

    pm.destroy(p->pid);
}

TEST(sbrk_stack_collision_returns_zero) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    PagingManager& paging = PagingManager::getManager();
    u32 pd = paging.createAddressSpace();
    ASSERT(pd != 0);

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, pd);
    ASSERT(p != nullptr);
    p->heapBreak = 0xBFFF0000;
    pm.setCurrent(p);

    // Would collide with user stack at 0xBFFFF000.
    u32 result = sh.sbrk(0x10000);
    ASSERT_EQ(result, 0u);
    ASSERT_EQ(p->heapBreak, 0xBFFF0000u); // Unchanged.

    paging.destroyAddressSpace(pd);
    pm.destroy(p->pid);
}
