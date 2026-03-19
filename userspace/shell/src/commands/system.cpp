/**
 * system.cpp -- System shell commands
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <std/os.hpp>
#include <std/ns.hpp>
#include <std/fmt.hpp>

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
    vga.clear();
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
    os::memInfo(total, used, free);

    char buf[48];
    print("Physical memory:\n");
    fmt::format(buf, sizeof(buf), "  Total: %u KiB (%u frames)\n", total * 4, total);
    print(buf);
    fmt::format(buf, sizeof(buf), "  Used:  %u KiB (%u frames)\n", used * 4, used);
    print(buf);
    fmt::format(buf, sizeof(buf), "  Free:  %u KiB (%u frames)\n", free * 4, free);
    print(buf);
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
    os::ProcEntry procs[16];
    u32 procCount = os::procList(procs, 16);

    ns::Entry names[16];
    u32 nameCount = ns::listAll(names, 16);

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

        char line[64];
        fmt::format(line, sizeof(line), "%3u  %-9s  %-14s  %u KB\n",
                    procs[i].pid, name, stateStr(procs[i].state),
                    procs[i].heapSize / 1024);
        print(line);
    }
}

void Shell::cmdUptime() {
    i32 ticks = os::uptime();
    u32 total_ms = (static_cast<u32>(ticks) * 1000) / os::TICK_FREQUENCY;
    u32 seconds = total_ms / 1000;
    u32 ms = total_ms % 1000;

    char buf[32];
    fmt::format(buf, sizeof(buf), "Up %u.%03us\n", seconds, ms);
    print(buf);
}

void Shell::cmdReboot() {
    print("Rebooting...\n");
    os::reboot();
}

void Shell::cmdShutdown() {
    print("Shutting down...\n");
    os::shutdown();
}
