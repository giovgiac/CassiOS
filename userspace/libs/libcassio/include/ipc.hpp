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

#include <types.hpp>
#include <syscall.hpp>
#include <message.hpp>

namespace cassio {

class IPC {
public:
    static inline i32 send(u32 pid, Message* msg,
                           const void* data = nullptr, u32 dataLen = 0) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Send), "b"(pid), "c"((u32)msg),
                       "S"((u32)data), "D"(dataLen)
                     : "memory");
        return ret;
    }

    static inline i32 receive(Message* msg,
                              void* data = nullptr, u32 dataCapacity = 0) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Receive), "b"((u32)msg),
                       "S"((u32)data), "D"(dataCapacity)
                     : "memory");
        return ret;
    }

    static inline i32 reply(u32 pid, Message* msg,
                            const void* data = nullptr, u32 dataLen = 0) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Reply), "b"(pid), "c"((u32)msg),
                       "S"((u32)data), "D"(dataLen)
                     : "memory");
        return ret;
    }

    static inline i32 notify(u32 pid, Message* msg,
                             const void* data = nullptr, u32 dataLen = 0) {
        i32 ret;
        asm volatile("int $0x80" : "=a"(ret)
                     : "a"(SyscallNumber::Notify), "b"(pid), "c"((u32)msg),
                       "S"((u32)data), "D"(dataLen)
                     : "memory");
        return ret;
    }
};

} // cassio

#endif // USERSPACE_LIB_IPC_HPP_
