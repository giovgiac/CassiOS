/**
 * system.hpp -- userspace system syscall wrappers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_LIB_SYSTEM_HPP_
#define USERSPACE_LIB_SYSTEM_HPP_

#include <types.hpp>
#include <syscall.hpp>

namespace cassio {

class System {
public:
    static inline i32 write(u32 fd, const char* buf, u32 len) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Write), "b"(fd), "c"((u32)buf), "d"(len)
                     : "memory");
        return ret;
    }

    static inline i32 sleep(u32 ms) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Sleep), "b"(ms)
                     : "memory");
        return ret;
    }

    static inline i32 uptime() {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Uptime)
                     : "memory");
        return ret;
    }

    static inline void reboot() {
        asm volatile("int $0x80" : : "a"(SyscallNumber::Reboot) : "memory");
    }

    static inline void shutdown() {
        asm volatile("int $0x80" : : "a"(SyscallNumber::Shutdown) : "memory");
    }

    static inline void memInfo(u32& total, u32& used, u32& free) {
        u32 t, u, f;
        asm volatile("int $0x80" : "=a"(t), "=b"(u), "=c"(f)
                     : "a"(SyscallNumber::MemInfo)
                     : "memory");
        total = t;
        used = u;
        free = f;
    }

    static inline void exit(u32 code) {
        asm volatile("int $0x80" : : "a"(SyscallNumber::Exit), "b"(code) : "memory");
    }

    static inline i32 irqRegister(u32 irq) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::IrqRegister), "b"(irq)
                     : "memory");
        return ret;
    }

    static inline i32 mapDevice(u32 physical, u32 virt, u32 pages) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::MapDevice), "b"(physical),
                       "c"(virt), "d"(pages)
                     : "memory");
        return ret;
    }
};

} // cassio

#endif // USERSPACE_LIB_SYSTEM_HPP_
