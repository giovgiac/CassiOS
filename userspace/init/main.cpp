/**
 * main.cpp -- init userspace program
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * First userspace process. Prints uptime every 5 seconds via syscalls.
 *
 */

#include <syscall.hpp>

using u32 = unsigned int;
using i32 = int;

static inline i32 syscall(u32 number, u32 arg1 = 0, u32 arg2 = 0, u32 arg3 = 0) {
    i32 ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return ret;
}

static i32 sys_write(u32 fd, const char* buf, u32 len) {
    return syscall(SyscallNumber::Write, fd, (u32)buf, len);
}

static i32 sys_sleep(u32 ms) {
    return syscall(SyscallNumber::Sleep, ms);
}

static i32 sys_uptime() {
    return syscall(SyscallNumber::Uptime);
}

static u32 strlen(const char* s) {
    u32 len = 0;
    while (s[len]) len++;
    return len;
}

static void print(const char* s) {
    sys_write(1, s, strlen(s));
}

static void print_dec(u32 value) {
    if (value == 0) {
        sys_write(1, "0", 1);
        return;
    }
    char buf[12];
    i32 i = 0;
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    // Reverse.
    for (i32 j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    sys_write(1, buf, i);
}

extern "C" void _start() {
    while (true) {
        u32 ticks = (u32)sys_uptime();
        print("init: alive, uptime = ");
        print_dec(ticks);
        print(" ticks\n");
        sys_sleep(5000);
    }
}
