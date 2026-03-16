/**
 * syscall.hpp -- userspace syscall wrappers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Inline assembly wrappers for int 0x80 syscalls.
 *
 */

#ifndef USERSPACE_LIB_SYSCALL_HPP_
#define USERSPACE_LIB_SYSCALL_HPP_

#include <syscall.hpp>
#include <message.hpp>

static inline int syscall(unsigned int number,
                          unsigned int arg1 = 0,
                          unsigned int arg2 = 0,
                          unsigned int arg3 = 0) {
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}

static inline int sys_send(unsigned int pid, Message* msg) {
    return syscall(SyscallNumber::Send, pid, (unsigned int)msg);
}

static inline int sys_receive(Message* msg) {
    return syscall(SyscallNumber::Receive, (unsigned int)msg);
}

static inline int sys_reply(unsigned int pid, Message* msg) {
    return syscall(SyscallNumber::Reply, pid, (unsigned int)msg);
}

static inline int sys_irq_register(unsigned int irq) {
    return syscall(SyscallNumber::IrqRegister, irq);
}

static inline int sys_write(unsigned int fd, const char* buf, unsigned int len) {
    return syscall(SyscallNumber::Write, fd, (unsigned int)buf, len);
}

static inline int sys_sleep(unsigned int ms) {
    return syscall(SyscallNumber::Sleep, ms);
}

static inline int sys_uptime() {
    return syscall(SyscallNumber::Uptime);
}

static inline void sys_reboot() {
    syscall(SyscallNumber::Reboot);
}

static inline void sys_shutdown() {
    syscall(SyscallNumber::Shutdown);
}

#endif // USERSPACE_LIB_SYSCALL_HPP_
