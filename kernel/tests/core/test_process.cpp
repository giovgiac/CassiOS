#include <core/process.hpp>
#include <std/test.hpp>

using namespace cassio;
using namespace std;
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

TEST(process_create_beyond_old_limit) {
    ProcessManager& pm = ProcessManager::getManager();
    // Should be able to create more than the old fixed limit of 15.
    Process* procs[20];
    for (u32 i = 0; i < 20; i++) {
        procs[i] = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
        ASSERT(procs[i] != nullptr);
    }
    // Verify all have unique PIDs and are accessible.
    for (u32 i = 0; i < 20; i++) {
        ASSERT(pm.get(procs[i]->pid) == procs[i]);
        for (u32 j = i + 1; j < 20; j++) {
            ASSERT(procs[i]->pid != procs[j]->pid);
        }
    }
    for (u32 i = 0; i < 20; i++) {
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
    // Before any scheduling, current() returns the kernel task (PID 0).
    Process* cur = pm.current();
    ASSERT(cur != nullptr);
    ASSERT(cur->state == ProcessState::Empty || cur->state == ProcessState::Running
           || cur->state == ProcessState::Ready);
}

TEST(process_get_returns_null_for_nonexistent) {
    ProcessManager& pm = ProcessManager::getManager();
    ASSERT(pm.get(99999) == nullptr);
}

TEST(process_destroy_wakes_send_blocked_senders) {
    ProcessManager& pm = ProcessManager::getManager();

    Process* target = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* sender = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(target != nullptr);
    ASSERT(sender != nullptr);

    // Simulate sender being SendBlocked on target.
    sender->state = ProcessState::SendBlocked;
    target->sendQueuePush(sender->pid);

    // Destroying target should wake the sender with Ready state.
    u32 targetPid = target->pid;
    pm.destroy(targetPid);

    ASSERT(sender->state == ProcessState::Ready);

    pm.destroy(sender->pid);
}
