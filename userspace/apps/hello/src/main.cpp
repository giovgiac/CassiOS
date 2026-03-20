/**
 * main.cpp -- Hello world user program
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/os.hpp>
#include <std/vga.hpp>

using namespace std;

extern "C" void _start() {
    vga::Vga vga;
    vga.write("Hello, World!\n");
    os::exit(0);
}
