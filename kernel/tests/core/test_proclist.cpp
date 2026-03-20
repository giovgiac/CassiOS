#include <std/test.hpp>

#include <core/process.hpp>
#include <core/syscall.hpp>
#include <memory/paging.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;
using namespace cassio::memory;

TEST(proclist_skips_kernel_task) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    os::ProcEntry buf[16];
    u32 count = sh.procList(buf, 16);

    // Should not include PID 0 (kernel task).
    for (u32 i = 0; i < count; ++i) {
        ASSERT(buf[i].pid != 0);
    }
}

TEST(proclist_includes_created_process) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);
    p->heapBase = 0x00500000;
    p->heapBreak = 0x00503000;

    os::ProcEntry buf[16];
    u32 count = sh.procList(buf, 16);

    bool found = false;
    for (u32 i = 0; i < count; ++i) {
        if (buf[i].pid == p->pid) {
            found = true;
            ASSERT_EQ(buf[i].state, static_cast<u32>(ProcessState::Ready));
            ASSERT_EQ(buf[i].heapSize, 0x3000u); // 3 pages = 12 KiB.
            break;
        }
    }
    ASSERT(found);

    pm.destroy(p->pid);
}

TEST(proclist_zero_heap_size_when_no_heap) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);
    // heapBase and heapBreak both 0 (default).

    os::ProcEntry buf[16];
    u32 count = sh.procList(buf, 16);

    for (u32 i = 0; i < count; ++i) {
        if (buf[i].pid == p->pid) {
            ASSERT_EQ(buf[i].heapSize, 0u);
            break;
        }
    }

    pm.destroy(p->pid);
}

TEST(proclist_respects_max_entries) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    ProcessManager& pm = ProcessManager::getManager();

    Process* p1 = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* p2 = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p1 != nullptr);
    ASSERT(p2 != nullptr);

    os::ProcEntry buf[1];
    u32 count = sh.procList(buf, 1);

    // Should return at most 1 entry.
    ASSERT_EQ(count, 1u);

    pm.destroy(p2->pid);
    pm.destroy(p1->pid);
}
