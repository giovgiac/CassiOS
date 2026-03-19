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

#include <std/types.hpp>
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
    static constexpr std::u32 DEFAULT_TIME_SLICE = 10;

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
    std::u32 schedule(std::u32 currentEsp);

    /**
     * @brief Immediate context switch without waiting for the time slice.
     *
     * Used by blocking syscalls (IPC send/receive) to yield the CPU.
     * Saves currentEsp into the current process and picks the next
     * Ready process. Does NOT change the current process's state --
     * the caller must set it (e.g. SendBlocked) before calling.
     *
     */
    std::u32 reschedule(std::u32 currentEsp);

    /**
     * @brief Destroys the current process and switches to the next ready one.
     *
     * Unlike reschedule(), does not save state into the dying process.
     * Finds the next ready process first, then destroys the current one
     * (address space, IPC queues, kernel stack), and switches directly.
     *
     */
    std::u32 exitCurrent(std::u32 currentEsp);

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
    std::u32 switchTo(Process* next, Process* current, std::u32 currentEsp);

    static Scheduler instance;

    GlobalDescriptorTable* gdt;
    std::u32 tickCount;
    std::u32 timeSlice;
};

} // kernel
} // cassio

#endif // CORE_SCHEDULER_HPP_
