/**
 * os.hpp -- OS interface: syscall constants and userspace wrappers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Syscall number constants (os::syscall) are shared between kernel
 * and userspace. The free functions provide userspace wrappers
 * around int 0x80.
 *
 */

#ifndef STD_OS_HPP
#define STD_OS_HPP

#include <std/types.hpp>

namespace std {
namespace os {

namespace syscall {
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
    constexpr u32 ProcList    = 14;
    constexpr u32 Count       = 15;
}

struct ProcEntry {
    u32 pid;
    u32 state;
    u32 heapSize;
};

i32 write(u32 fd, const char* buf, u32 len);
i32 sleep(u32 ms);
i32 uptime();
void reboot();
void shutdown();
void memInfo(u32& total, u32& used, u32& free);
void exit(u32 code);
i32 irqRegister(u32 irq);
i32 mapDevice(u32 physical, u32 virt, u32 pages);
void* sbrk(u32 increment);
u32 procList(ProcEntry* buf, u32 maxEntries);

}
}

#endif // STD_OS_HPP
