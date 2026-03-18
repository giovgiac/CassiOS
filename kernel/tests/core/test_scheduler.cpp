#include <core/scheduler.hpp>
#include <test.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;

extern GlobalDescriptorTable test_gdt;

TEST(scheduler_single_process_returns_same_esp) {
    Scheduler& s = Scheduler::getScheduler();
    s.reset();
    s.init(test_gdt);

    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    p->state = ProcessState::Running;
    pm.setCurrent(p);

    // With only one schedulable process, schedule always returns the same ESP.
    u32 result = s.schedule(0x5000);
    ASSERT_EQ(result, 0x5000u);

    result = s.schedule(0x6000);
    ASSERT_EQ(result, 0x6000u);

    pm.destroy(p->pid);
}

TEST(scheduler_two_processes_switch_after_time_slice) {
    Scheduler& s = Scheduler::getScheduler();
    s.reset();
    s.init(test_gdt);

    ProcessManager& pm = ProcessManager::getManager();
    Process* p1 = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    p1->state = ProcessState::Running;
    pm.setCurrent(p1);

    Process* p2 = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);

    // Before time slice expires, schedule returns the same ESP.
    for (u32 i = 0; i < Scheduler::DEFAULT_TIME_SLICE - 1; i++) {
        u32 result = s.schedule(0x5000);
        ASSERT_EQ(result, 0x5000u);
    }

    // Time slice expires: scheduler saves 0x5000 into p1 and returns p2's ESP.
    u32 result = s.schedule(0x5000);
    ASSERT_EQ(result, 0x4000u);

    pm.destroy(p1->pid);
    pm.destroy(p2->pid);
}
