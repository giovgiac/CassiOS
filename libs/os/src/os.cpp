/**
 * os.cpp -- Userspace syscall wrappers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/os.hpp>

using namespace std;

i32 os::write(u32 fd, const char* buf, u32 len) {
    i32 ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(os::syscall::Write), "b"(fd), "c"((u32)buf), "d"(len)
                 : "memory");
    return ret;
}

i32 os::sleep(u32 ms) {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(os::syscall::Sleep), "b"(ms) : "memory");
    return ret;
}

i32 os::uptime() {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(os::syscall::Uptime) : "memory");
    return ret;
}

void os::reboot() {
    asm volatile("int $0x80" : : "a"(os::syscall::Reboot) : "memory");
}

void os::shutdown() {
    asm volatile("int $0x80" : : "a"(os::syscall::Shutdown) : "memory");
}

void os::memInfo(u32& total, u32& used, u32& free) {
    u32 t, u, f;
    asm volatile("int $0x80" : "=a"(t), "=b"(u), "=c"(f) : "a"(os::syscall::MemInfo) : "memory");
    total = t;
    used = u;
    free = f;
}

void os::exit(u32 code) {
    asm volatile("int $0x80" : : "a"(os::syscall::Exit), "b"(code) : "memory");
}

i32 os::irqRegister(u32 irq) {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(os::syscall::IrqRegister), "b"(irq) : "memory");
    return ret;
}

i32 os::mapDevice(u32 physical, u32 virt, u32 pages) {
    i32 ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(os::syscall::MapDevice), "b"(physical), "c"(virt), "d"(pages)
                 : "memory");
    return ret;
}

void* os::sbrk(u32 increment) {
    u32 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(os::syscall::Sbrk), "b"(increment) : "memory");
    return reinterpret_cast<void*>(ret);
}

u32 os::procList(ProcEntry* buf, u32 maxEntries) {
    u32 ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(os::syscall::ProcList), "b"((u32)buf), "c"(maxEntries)
                 : "memory");
    return ret;
}

u32 os::exec(const void* elfData, u32 size) {
    u32 ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(os::syscall::Exec), "b"((u32)elfData), "c"(size)
                 : "memory");
    return ret;
}

i32 os::waitpid(u32 pid) {
    i32 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(os::syscall::WaitPid), "b"(pid) : "memory");
    return ret;
}
