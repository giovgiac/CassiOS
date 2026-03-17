/**
 * scheduler.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_SCHEDULER_HPP_
#define CORE_SCHEDULER_HPP_

#include <types.hpp>
#include <core/gdt.hpp>
#include <core/process.hpp>

namespace cassio {
namespace kernel {

/**
 * @brief Preemptive round-robin scheduler driven by the PIT timer.
 *
 * Iterates the ProcessManager's linked list to find the next Ready
 * process. Context switching is done by returning a different ESP
 * from schedule(), which the interrupt stub uses to restore the
 * next process's register state.
 *
 */
class Scheduler {
public:
    static constexpr u32 DEFAULT_TIME_SLICE = 10;

    inline static Scheduler& getScheduler() {
        return instance;
    }

    /**
     * @brief Stores a reference to the GDT for TSS.esp0 updates.
     *
     */
    void init(GlobalDescriptorTable& gdt);

    /**
     * @brief Called from the PIT handler on every tick. Returns the ESP
     *        to restore -- either the same (no switch) or a different
     *        process's saved ESP (context switch).
     *
     */
    u32 schedule(u32 currentEsp);

    /**
     * @brief Immediate context switch without waiting for the time slice.
     *
     * Used by blocking syscalls (IPC send/receive) to yield the CPU.
     * Saves currentEsp into the current process and picks the next
     * Ready process. Does NOT change the current process's state --
     * the caller must set it (e.g. SendBlocked) before calling.
     *
     */
    u32 reschedule(u32 currentEsp);

    /**
     * @brief Resets scheduler state. Used by the test framework.
     *
     */
    void reset();

    /** Deleted Methods */
    Scheduler(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

private:
    Scheduler();

    Process* findNextReady(Process* current);
    u32 switchTo(Process* next, Process* current, u32 currentEsp);

    static Scheduler instance;

    GlobalDescriptorTable* gdt;
    u32 tickCount;
    u32 timeSlice;
};

} // kernel
} // cassio

#endif // CORE_SCHEDULER_HPP_
