/**
 * process.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Processes, the scheduler and syscalls are Rust (kernel/src/core/).
 * ProcessManager forwards to the entry points start() and the test kernel
 * use until they are ported too.
 *
 */

#ifndef CORE_PROCESS_HPP_
#define CORE_PROCESS_HPP_

#include <std/types.hpp>

/** Adds the kernel task (PID 0), running. */
extern "C" void process_init();

/** Starts a loaded module; returns its PID, or 0 on failure. */
extern "C" std::u32 process_spawn(std::u32 directory, std::u32 entry, std::u32 heapStart,
                                  std::u32 cs, std::u32 ds);

/** Records the multiboot framebuffer for the FramebufferInfo syscall. */
extern "C" void syscall_set_framebuffer(std::u32 address, std::u32 width, std::u32 height,
                                        std::u32 pitch, std::u32 bpp);

namespace cassio {
namespace kernel {

class ProcessManager final {
public:
    /** Adds the kernel task (PID 0), running. */
    static void init() { process_init(); }

    /** Starts a loaded module; returns its PID, or 0 on failure. */
    static std::u32 spawn(std::u32 directory, std::u32 entry, std::u32 heapStart, std::u32 cs,
                          std::u32 ds) {
        return process_spawn(directory, entry, heapStart, cs, ds);
    }

    /** Records the multiboot framebuffer for the FramebufferInfo syscall. */
    static void setFramebuffer(std::u32 address, std::u32 width, std::u32 height, std::u32 pitch,
                               std::u32 bpp) {
        syscall_set_framebuffer(address, width, height, pitch, bpp);
    }

    ProcessManager() = delete;
};

} // namespace kernel
} // namespace cassio

#endif // CORE_PROCESS_HPP_
