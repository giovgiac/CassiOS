/**
 * ipc.hpp -- userspace IPC syscall wrappers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_LIB_IPC_HPP_
#define USERSPACE_LIB_IPC_HPP_

#include <std/types.hpp>
#include <syscall.hpp>
#include <std/msg.hpp>

namespace cassio {

class IPC {
public:
    static inline std::i32 send(std::u32 pid, std::msg::Message* msg,
                           const void* data = nullptr, std::u32 dataLen = 0) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Send), "b"(pid), "c"((std::u32)msg),
                       "S"((std::u32)data), "D"(dataLen)
                     : "memory");
        return ret;
    }

    static inline std::i32 receive(std::msg::Message* msg,
                              void* data = nullptr, std::u32 dataCapacity = 0) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Receive), "b"((std::u32)msg),
                       "S"((std::u32)data), "D"(dataCapacity)
                     : "memory");
        return ret;
    }

    static inline std::i32 reply(std::u32 pid, std::msg::Message* msg,
                            const void* data = nullptr, std::u32 dataLen = 0) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Reply), "b"(pid), "c"((std::u32)msg),
                       "S"((std::u32)data), "D"(dataLen)
                     : "memory");
        return ret;
    }

    static inline std::i32 notify(std::u32 pid, std::msg::Message* msg,
                             const void* data = nullptr, std::u32 dataLen = 0) {
        std::i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Notify), "b"(pid), "c"((std::u32)msg),
                       "S"((std::u32)data), "D"(dataLen)
                     : "memory");
        return ret;
    }
};

} // cassio

#endif // USERSPACE_LIB_IPC_HPP_
