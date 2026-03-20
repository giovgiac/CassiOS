/**
 * system.cpp -- System shell commands
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/fmt.hpp>
#include <std/heap.hpp>
#include <std/ns.hpp>
#include <std/os.hpp>

#include <shell.hpp>

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
    print("  exec        - Execute a program\n");
}

void Shell::cmdClear() {
    vga.clear();
}

void Shell::cmdEcho(const char** args, u8 argc) {
    for (u8 i = 1; i < argc; ++i) {
        if (i > 1)
            putchar(' ');
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
    case 1:
        return "Ready";
    case 2:
        return "Running";
    case 3:
        return "SendBlocked";
    case 4:
        return "ReceiveBlocked";
    case 5:
        return "WaitBlocked";
    default:
        return "Unknown";
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
        fmt::format(line, sizeof(line), "%3u  %-9s  %-14s  %u KB\n", procs[i].pid, name,
                    stateStr(procs[i].state), procs[i].heapSize / 1024);
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

void Shell::cmdExec(const char** args, u8 argc) {
    if (argc < 2) {
        print("exec: usage: exec <path>\n");
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    u32 handle = vfs.open(path);
    if (handle == 0) {
        print("exec: no such file: ");
        print(args[1]);
        putchar('\n');
        return;
    }

    // Read the entire file into a heap buffer.
    static constexpr u32 MAX_ELF_SIZE = 65536;
    u8* elfBuf = static_cast<u8*>(heap::alloc(MAX_ELF_SIZE));
    if (!elfBuf) {
        print("exec: out of memory\n");
        return;
    }

    u32 total = 0;
    while (total < MAX_ELF_SIZE) {
        i32 n = vfs.read(handle, total, elfBuf + total, 512);
        if (n <= 0)
            break;
        total += static_cast<u32>(n);
    }

    if (total == 0) {
        print("exec: empty file\n");
        heap::free(elfBuf);
        return;
    }

    u32 childPid = os::exec(elfBuf, total);
    heap::free(elfBuf);

    if (childPid == 0) {
        print("exec: failed to load program\n");
        return;
    }

    os::waitpid(childPid);
}
