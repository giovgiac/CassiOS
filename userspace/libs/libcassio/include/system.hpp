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

#include <std/types.hpp>
#include <syscall.hpp>

namespace cassio {

class System {
public:
    static inline std::i32 write(std::u32 fd, const char* buf, std::u32 len) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Write), "b"(fd), "c"((std::u32)buf), "d"(len)
                     : "memory");
        return ret;
    }

    static inline std::i32 sleep(std::u32 ms) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Sleep), "b"(ms)
                     : "memory");
        return ret;
    }

    static inline std::i32 uptime() {
        std::i32 ret;
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

    static inline void memInfo(std::u32& total, std::u32& used, std::u32& free) {
        std::u32 t, u, f;
        asm volatile("int $0x80" : "=a"(t), "=b"(u), "=c"(f)
                     : "a"(SyscallNumber::MemInfo)
                     : "memory");
        total = t;
        used = u;
        free = f;
    }

    static inline void exit(std::u32 code) {
        asm volatile("int $0x80" : : "a"(SyscallNumber::Exit), "b"(code) : "memory");
    }

    static inline std::i32 irqRegister(std::u32 irq) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::IrqRegister), "b"(irq)
                     : "memory");
        return ret;
    }

    static inline std::i32 mapDevice(std::u32 physical, std::u32 virt, std::u32 pages) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::MapDevice), "b"(physical),
                       "c"(virt), "d"(pages)
                     : "memory");
        return ret;
    }

    static inline void* sbrk(std::u32 increment) {
        std::u32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Sbrk), "b"(increment)
                     : "memory");
        return (void*)ret;
    }

    static inline std::u32 procList(ProcEntry* buf, std::u32 maxEntries) {
        std::u32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::ProcList), "b"((std::u32)buf), "c"(maxEntries)
                     : "memory");
        return ret;
    }
};

} // cassio

#endif // USERSPACE_LIB_SYSTEM_HPP_
