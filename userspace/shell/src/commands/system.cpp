/**
 * system.cpp -- System shell commands
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <timer.hpp>
#include <vga.hpp>
#include <system.hpp>
#include <ns.hpp>
#include <std/str.hpp>

using namespace cassio;
using namespace std;

void Shell::cmdHelp() {
    print("Available commands:\n");
    print("  help        - Show this help\n");
    print("  clear       - Clear the screen\n");
    print("  echo        - Print text\n");
    print("  mem         - Show memory statistics\n");
    print("  ps          - List running processes\n");
    print("  uptime      - Show time since boot\n");
    print("  reboot      - Reboot the system\n");
    print("  shutdown    - Halt the system\n");
    print("  ls          - List directory\n");
    print("  cd          - Change directory\n");
    print("  pwd         - Print working directory\n");
    print("  mkdir       - Create directory\n");
    print("  rmdir       - Remove directory\n");
    print("  touch       - Create empty file\n");
    print("  rm          - Remove file\n");
    print("  cat         - Print file contents\n");
    print("  write       - Write text to file\n");
}

void Shell::cmdClear() {
    Vga::clear(vgaPid);
}

void Shell::cmdEcho(const char** args, u8 argc) {
    for (u8 i = 1; i < argc; ++i) {
        if (i > 1) putchar(' ');
        print(args[i]);
    }
    putchar('\n');
}

void Shell::cmdMem() {
    u32 total, used, free;
    System::memInfo(total, used, free);

    print("Physical memory:\n");
    print("  Total: ");
    printDec(total * 4);
    print(" KiB (");
    printDec(total);
    print(" frames)\n");
    print("  Used:  ");
    printDec(used * 4);
    print(" KiB (");
    printDec(used);
    print(" frames)\n");
    print("  Free:  ");
    printDec(free * 4);
    print(" KiB (");
    printDec(free);
    print(" frames)\n");
}

static const char* stateStr(u32 state) {
    switch (state) {
    case 1: return "Ready";
    case 2: return "Running";
    case 3: return "SendBlocked";
    case 4: return "ReceiveBlocked";
    default: return "Unknown";
    }
}

void Shell::cmdPs() {
    ProcEntry procs[16];
    u32 procCount = System::procList(procs, 16);

    NsEntry names[16];
    u32 nameCount = Nameserver::listAll(names, 16);

    print("PID  NAME       STATE           HEAP\n");

    for (u32 i = 0; i < procCount; ++i) {
        // Find name for this PID.
        const char* name = "<unknown>";
        for (u32 j = 0; j < nameCount; ++j) {
            if (names[j].pid == procs[i].pid) {
                name = names[j].name;
                break;
            }
        }

        // PID (right-aligned, 3 chars).
        if (procs[i].pid < 10) print("  ");
        else if (procs[i].pid < 100) print(" ");
        printDec(procs[i].pid);
        print("  ");

        // NAME (left-aligned, 9 chars).
        print(name);
        u32 nameLen = str::len(name);
        for (u32 pad = nameLen; pad < 9; ++pad) {
            putchar(' ');
        }
        print("  ");

        // STATE (left-aligned, 14 chars).
        const char* state = stateStr(procs[i].state);
        print(state);
        u32 stateLen = str::len(state);
        for (u32 pad = stateLen; pad < 14; ++pad) {
            putchar(' ');
        }
        print("  ");

        // HEAP (in KB).
        printDec(procs[i].heapSize / 1024);
        print(" KB\n");
    }
}

void Shell::cmdUptime() {
    i32 ticks = System::uptime();
    u32 total_ms = (static_cast<u32>(ticks) * 1000) / TICK_FREQUENCY;
    u32 seconds = total_ms / 1000;
    u32 ms = total_ms % 1000;

    print("Up ");
    printDec(seconds);
    putchar('.');
    if (ms < 100) putchar('0');
    if (ms < 10) putchar('0');
    printDec(ms);
    print("s\n");
}

void Shell::cmdReboot() {
    print("Rebooting...\n");
    System::reboot();
}

void Shell::cmdShutdown() {
    print("Shutting down...\n");
    System::shutdown();
}
