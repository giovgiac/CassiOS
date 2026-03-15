/**
 * syscall.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_SYSCALL_HPP_
#define CORE_SYSCALL_HPP_

#include <common/types.hpp>

namespace cassio {
namespace kernel {

namespace SyscallNumber {
    constexpr u32 Write  = 0;
    constexpr u32 Read   = 1;
    constexpr u32 Sleep  = 2;
    constexpr u32 Uptime = 3;
    constexpr u32 Count  = 4;
}

/**
 * @brief Singleton that handles int 0x80 syscall dispatch.
 *
 * Registers a DPL=3 interrupt gate at vector 0x80 and routes syscalls
 * to private handler methods based on the syscall number in EAX.
 *
 */
class SyscallHandler {
private:
    static SyscallHandler instance;

private:
    SyscallHandler();
    ~SyscallHandler() = default;

    i32 write(u32 fd, u32 buf, u32 len);
    i32 read(u32 fd, u32 buf, u32 len);
    i32 sleep(u32 ms);
    i32 uptime();

public:
    /**
     * @brief Returns the singleton SyscallHandler instance.
     *
     */
    inline static SyscallHandler& getSyscallHandler() {
        return instance;
    }

    /**
     * @brief Registers the syscall gate at vector 0x80 with DPL=3.
     *
     */
    void load();

    /**
     * @brief Dispatches a syscall by number, returning the result in EAX.
     *
     */
    i32 handleSyscall(u32 number, u32 ebx, u32 ecx, u32 edx);

    /** Deleted Methods */
    SyscallHandler(const SyscallHandler&) = delete;
    SyscallHandler(SyscallHandler&&) = delete;
    SyscallHandler& operator=(const SyscallHandler&) = delete;
    SyscallHandler& operator=(SyscallHandler&&) = delete;
};

} // kernel
} // cassio

/**
 * @brief C-linkage syscall handler called from the assembly stub in syscall_stub.s.
 *
 */
extern "C" cassio::i32 handleSyscall(cassio::u32 eax, cassio::u32 ebx, cassio::u32 ecx, cassio::u32 edx);

#endif // CORE_SYSCALL_HPP_
