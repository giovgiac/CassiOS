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

#include <types.hpp>
#include <ns.hpp>
#include <shell.hpp>

using namespace cassio;

extern "C" void _start() {
    // Wait for required services to register.
    u32 kbdPid = 0;
    while (kbdPid == 0) {
        kbdPid = Nameserver::lookup("kbd");
    }

    u32 vgaPid = 0;
    while (vgaPid == 0) {
        vgaPid = Nameserver::lookup("vga");
    }

    u32 vfsPid = 0;
    while (vfsPid == 0) {
        vfsPid = Nameserver::lookup("vfs");
    }

    Shell shell(kbdPid, vgaPid, vfsPid);
    shell.run();
}
