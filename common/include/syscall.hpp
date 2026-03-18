/**
 * syscall.hpp -- syscall number constants
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_SYSCALL_HPP_
#define COMMON_SYSCALL_HPP_

#include <std/types.hpp>

namespace cassio {

namespace SyscallNumber {
    constexpr std::u32 Send        = 0;
    constexpr std::u32 Receive     = 1;
    constexpr std::u32 Reply       = 2;
    constexpr std::u32 IrqRegister = 3;
    constexpr std::u32 Write       = 4;
    constexpr std::u32 Sleep       = 5;
    constexpr std::u32 Uptime      = 6;
    constexpr std::u32 Reboot      = 7;
    constexpr std::u32 Shutdown    = 8;
    constexpr std::u32 Exit        = 9;
    constexpr std::u32 MapDevice   = 10;
    constexpr std::u32 Notify      = 11;
    constexpr std::u32 MemInfo     = 12;
    constexpr std::u32 Sbrk        = 13;
    constexpr std::u32 ProcList    = 14;
    constexpr std::u32 Count       = 15;
}

struct ProcEntry {
    std::u32 pid;
    std::u32 state;
    std::u32 heapSize;
};

} // cassio

#endif // COMMON_SYSCALL_HPP_
