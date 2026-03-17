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

#include <types.hpp>

namespace cassio {

namespace SyscallNumber {
    constexpr u32 Send        = 0;
    constexpr u32 Receive     = 1;
    constexpr u32 Reply       = 2;
    constexpr u32 IrqRegister = 3;
    constexpr u32 Write       = 4;
    constexpr u32 Sleep       = 5;
    constexpr u32 Uptime      = 6;
    constexpr u32 Reboot      = 7;
    constexpr u32 Shutdown    = 8;
    constexpr u32 Exit        = 9;
    constexpr u32 MapDevice   = 10;
    constexpr u32 Notify      = 11;
    constexpr u32 MemInfo     = 12;
    constexpr u32 Sbrk        = 13;
    constexpr u32 Count       = 14;
}

} // cassio

#endif // COMMON_SYSCALL_HPP_
