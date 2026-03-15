#include <core/process.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::kernel;

TEST(process_create_assigns_pid) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);
    ASSERT(p->pid > 0);
    ASSERT(p->state == ProcessState::Ready);
    ASSERT_EQ(p->eip, 0x1000u);
    ASSERT_EQ(p->esp, 0x2000u);
    ASSERT_EQ(p->cs, 0x08u);
    ASSERT_EQ(p->ds, 0x10u);
    ASSERT_EQ(p->eflags, 0x202u);
    pm.destroy(p->pid);
}

TEST(process_create_unique_pids) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p1 = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* p2 = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(p1 != nullptr);
    ASSERT(p2 != nullptr);
    ASSERT(p1->pid != p2->pid);
    pm.destroy(p1->pid);
    pm.destroy(p2->pid);
}

TEST(process_create_fills_table) {
    ProcessManager& pm = ProcessManager::getManager();
    // Slots 1..15 = 15 available slots (slot 0 is reserved for kernel task).
    Process* procs[15];
    for (u32 i = 0; i < 15; i++) {
        procs[i] = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
        ASSERT(procs[i] != nullptr);
    }
    // Table is full, next create should return null.
    Process* overflow = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(overflow == nullptr);
    // Clean up.
    for (u32 i = 0; i < 15; i++) {
        pm.destroy(procs[i]->pid);
    }
}

TEST(process_destroy_frees_slot) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);
    u32 pid = p->pid;
    pm.destroy(pid);
    // After destroy, get() should return null.
    ASSERT(pm.get(pid) == nullptr);
}

TEST(process_current_returns_kernel_task) {
    ProcessManager& pm = ProcessManager::getManager();
    // Before any scheduling, current() returns the kernel task (slot 0).
    Process* cur = pm.current();
    ASSERT(cur != nullptr);
    ASSERT(cur->state == ProcessState::Empty || cur->state == ProcessState::Running
           || cur->state == ProcessState::Ready);
}

TEST(process_get_returns_null_for_nonexistent) {
    ProcessManager& pm = ProcessManager::getManager();
    ASSERT(pm.get(99999) == nullptr);
}
