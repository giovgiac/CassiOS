/**
 * ipc.cpp -- IPC syscall wrappers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ipc.hpp>
#include <std/os.hpp>

using namespace std;

i32 ipc::send(u32 pid, ipc::Message* msg,
              const void* data, u32 dataLen) {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret)
                 : "a"(os::syscall::Send), "b"(pid), "c"((u32)msg),
                   "S"((u32)data), "D"(dataLen)
                 : "memory");
    return ret;
}

i32 ipc::receive(ipc::Message* msg,
                 void* data, u32 dataCapacity) {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret)
                 : "a"(os::syscall::Receive), "b"((u32)msg),
                   "S"((u32)data), "D"(dataCapacity)
                 : "memory");
    return ret;
}

i32 ipc::reply(u32 pid, ipc::Message* msg,
               const void* data, u32 dataLen) {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret)
                 : "a"(os::syscall::Reply), "b"(pid), "c"((u32)msg),
                   "S"((u32)data), "D"(dataLen)
                 : "memory");
    return ret;
}

i32 ipc::notify(u32 pid, ipc::Message* msg,
                const void* data, u32 dataLen) {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret)
                 : "a"(os::syscall::Notify), "b"(pid), "c"((u32)msg),
                   "S"((u32)data), "D"(dataLen)
                 : "memory");
    return ret;
}
