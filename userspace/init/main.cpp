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

#include <types.hpp>
#include <string.hpp>
#include <system.hpp>

using namespace cassio;

static void print(const char* s) {
    System::write(1, s, strlen(s));
}

static void print_dec(u32 value) {
    if (value == 0) {
        System::write(1, "0", 1);
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
    System::write(1, buf, i);
}

extern "C" void _start() {
    while (true) {
        u32 ticks = static_cast<u32>(System::uptime());
        print("init: alive, uptime = ");
        print_dec(ticks);
        print(" ticks\n");
        System::sleep(5000);
    }
}
