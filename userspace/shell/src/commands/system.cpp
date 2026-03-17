/**
 * system.cpp -- System shell commands
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <vga.hpp>
#include <system.hpp>

using namespace cassio;

void Shell::cmdHelp() {
    print("Available commands:\n");
    print("  help        - Show this help\n");
    print("  clear       - Clear the screen\n");
    print("  echo        - Print text\n");
    print("  mem         - Show memory statistics\n");
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

void Shell::cmdUptime() {
    i32 ticks = System::uptime();
    u32 total_ms = (static_cast<u32>(ticks) * 1000) / 100;
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
