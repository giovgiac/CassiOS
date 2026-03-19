/**
 * main.cpp -- Shell service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace shell process. Reads keyboard input and dispatches
 * commands to VGA, VFS, and other services via IPC.
 *
 */

#include <std/ns.hpp>
#include <shell.hpp>

using namespace cassio;
using namespace std;

extern "C" void _start() {
    ns::registerName("shell");

    Shell shell;
    shell.run();
}
