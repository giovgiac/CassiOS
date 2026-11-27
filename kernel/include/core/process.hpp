/**
 * process.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Processes, the scheduler and syscalls are Rust (kernel/src/core/). These
 * are the entry points start() and the test kernel use until they are
 * ported too.
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

#endif // CORE_PROCESS_HPP_
