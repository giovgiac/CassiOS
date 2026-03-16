/**
 * syscall.hpp -- syscall number constants
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Shared between kernel and userspace. No namespace or kernel-specific types.
 *
 */

#ifndef SHARED_SYSCALL_HPP_
#define SHARED_SYSCALL_HPP_

namespace SyscallNumber {
    constexpr unsigned int Send        = 0;
    constexpr unsigned int Receive     = 1;
    constexpr unsigned int Reply       = 2;
    constexpr unsigned int IrqRegister = 3;
    constexpr unsigned int Write       = 4;
    constexpr unsigned int Sleep       = 5;
    constexpr unsigned int Uptime      = 6;
    constexpr unsigned int Reboot      = 7;
    constexpr unsigned int Shutdown    = 8;
    constexpr unsigned int Count       = 9;
}

#endif // SHARED_SYSCALL_HPP_
