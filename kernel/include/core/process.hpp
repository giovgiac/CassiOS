/**
 * process.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_PROCESS_HPP_
#define CORE_PROCESS_HPP_

#include <common/types.hpp>

namespace cassio {
namespace kernel {

enum class ProcessState : u8 {
    Empty,
    Ready,
    Running,
};

struct Process {
    u32 pid;
    ProcessState state;

    u32 eax, ebx, ecx, edx;
    u32 esi, edi, ebp, esp;
    u32 eip;
    u32 eflags;
    u32 cs, ds;

    u32 pageDirectory;
    u32 kernelEsp;
};

/**
 * @brief Singleton that manages the process table.
 *
 * Owns a fixed-size array of Process slots. PID 0 is reserved for the
 * kernel task and is initialized directly, not via create().
 *
 */
class ProcessManager {
public:
    static constexpr u32 MAX_PROCESSES = 16;

    inline static ProcessManager& getManager() {
        return instance;
    }

    /**
     * @brief Creates a new process in the first available slot.
     *
     * Returns null if no slots are available.
     *
     */
    Process* create(u32 eip, u32 esp, u32 cs, u32 ds, u32 pageDirectory);

    /**
     * @brief Destroys a process by marking its slot as Empty.
     *
     */
    void destroy(u32 pid);

    /**
     * @brief Returns the currently running process.
     *
     */
    Process* current();

    /**
     * @brief Returns the process at the given index, or null if out of range.
     *
     */
    Process* get(u32 pid);

    /**
     * @brief Sets the given process as the current one.
     *
     */
    void setCurrent(Process* process);

    /** Deleted Methods */
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

private:
    ProcessManager();

    static ProcessManager instance;

    Process processes[MAX_PROCESSES];
    u32 currentPid;
    u32 nextPid;
};

} // kernel
} // cassio

#endif // CORE_PROCESS_HPP_
