/**
 * syscall.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/syscall.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/pit.hpp"
#include "hardware/interrupt.hpp"
#include "hardware/serial.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::drivers;
using namespace cassio::hardware;

extern "C" void syscallEntry();

SyscallHandler SyscallHandler::instance;

SyscallHandler::SyscallHandler() {}

void SyscallHandler::load() {
    InterruptManager& im = InterruptManager::getManager();
    im.setGate(0x80, &syscallEntry, 3);
}

i32 SyscallHandler::handleSyscall(u32 number, u32 ebx, u32 ecx, u32 edx) {
    switch (number) {
    case SyscallNumber::Write:  return write(ebx, ecx, edx);
    case SyscallNumber::Read:   return read(ebx, ecx, edx);
    case SyscallNumber::Sleep:  return sleep(ebx);
    case SyscallNumber::Uptime: return uptime();
    default:                    return -1;
    }
}

i32 SyscallHandler::write(u32 fd, u32 buf, u32 len) {
    const char* str = reinterpret_cast<const char*>(buf);

    if (fd == 1) {
        VgaTerminal& vga = VgaTerminal::getTerminal();
        for (u32 i = 0; i < len; ++i) {
            vga.putchar(str[i]);
        }
        return static_cast<i32>(len);
    }

    if (fd == 2) {
        Serial& serial = COM1::getSerial();
        for (u32 i = 0; i < len; ++i) {
            serial.putchar(str[i]);
        }
        return static_cast<i32>(len);
    }

    return -1;
}

i32 SyscallHandler::read(u32 fd, u32 buf, u32 len) {
    if (fd == 0) {
        KeyboardDriver& kb = KeyboardDriver::getDriver();
        char* dst = reinterpret_cast<char*>(buf);
        u32 count = 0;
        while (count < len) {
            char ch = kb.readBuffer();
            if (ch == '\0') {
                break;
            }
            dst[count++] = ch;
        }
        return static_cast<i32>(count);
    }

    return -1;
}

i32 SyscallHandler::sleep(u32 ms) {
    PitTimer& pit = PitTimer::getTimer();
    pit.sleep(ms);
    return 0;
}

i32 SyscallHandler::uptime() {
    PitTimer& pit = PitTimer::getTimer();
    return static_cast<i32>(pit.getTicks());
}

i32 handleSyscall(u32 eax, u32 ebx, u32 ecx, u32 edx) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    return sh.handleSyscall(eax, ebx, ecx, edx);
}
