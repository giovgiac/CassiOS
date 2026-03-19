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

#include <std/types.hpp>
#include <std/ns.hpp>
#include <shell.hpp>

using namespace cassio;
using namespace std;

extern "C" void _start() {
    // Wait for required services to register.
    u32 kbdPid = 0;
    while (kbdPid == 0) {
        kbdPid = ns::lookup("kbd");
    }

    u32 vfsPid = 0;
    while (vfsPid == 0) {
        vfsPid = ns::lookup("vfs");
    }

    ns::registerName("shell");

    Shell shell(kbdPid, vfsPid);
    shell.run();
}
