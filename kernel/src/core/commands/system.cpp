/**
 * system.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/system.hpp"
#include <string.hpp>
#include "core/syscall.hpp"
#include "drivers/pit.hpp"
#include "hardware/port.hpp"
#include "hardware/terminal.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;
using namespace cassio::memory;

// --- help ---

static HelpCommand helpInstance;

HelpCommand::HelpCommand() : Command("help", "Show available commands") {}

bool HelpCommand::execute(const char** args, usize argc,
                          FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.print("Available commands:\n");

    Command** cmds = Command::getRegistry();
    u8 count = Command::getCount();
    for (u8 i = 0; i < count; ++i) {
        vga.print("  ");
        vga.print(cmds[i]->getName());

        // Pad to 12 characters for alignment.
        usize len = strlen(cmds[i]->getName());
        for (usize j = len; j < 12; ++j) {
            vga.putchar(' ');
        }

        vga.print("- ");
        vga.print(cmds[i]->getDescription());
        vga.putchar('\n');
    }

    return true;
}

// --- clear ---

static ClearCommand clearInstance;

ClearCommand::ClearCommand() : Command("clear", "Clear the screen") {}

bool ClearCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal::getTerminal().clear();
    return true;
}

// --- mem ---

static MemCommand memInstance;

MemCommand::MemCommand() : Command("mem", "Show memory statistics") {}

bool MemCommand::execute(const char** args, usize argc,
                         FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    u32 free = pmm.getFreeFrames();
    u32 total = pmm.getTotalFrames();
    u32 used = pmm.getUsedFrames();

    vga.print("Physical memory:\n");
    vga.print("  Total: ");
    vga.print_dec(total * 4);
    vga.print(" KiB (");
    vga.print_dec(total);
    vga.print(" frames)\n");
    vga.print("  Used:  ");
    vga.print_dec(used * 4);
    vga.print(" KiB (");
    vga.print_dec(used);
    vga.print(" frames)\n");
    vga.print("  Free:  ");
    vga.print_dec(free * 4);
    vga.print(" KiB (");
    vga.print_dec(free);
    vga.print(" frames)\n");

    return true;
}

// --- uptime ---

static UptimeCommand uptimeInstance;

UptimeCommand::UptimeCommand() : Command("uptime", "Show time since boot") {}

bool UptimeCommand::execute(const char** args, usize argc,
                            FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    PitTimer& pit = PitTimer::getTimer();
    u32 ticks = pit.getTicks();
    u32 total_ms = (ticks * 1000) / PIT_FREQUENCY;
    u32 seconds = total_ms / 1000;
    u32 ms = total_ms % 1000;

    vga.print("Up ");
    vga.print_dec(seconds);
    vga.putchar('.');

    // Print milliseconds with leading zeros (3 digits).
    if (ms < 100) vga.putchar('0');
    if (ms < 10) vga.putchar('0');
    vga.print_dec(ms);
    vga.print("s\n");

    return true;
}

// --- reboot ---

static RebootCommand rebootInstance;

RebootCommand::RebootCommand() : Command("reboot", "Reboot the system") {}

bool RebootCommand::execute(const char** args, usize argc,
                            FileNode*& cwd) {
    VgaTerminal::getTerminal().print("Rebooting...\n");
    Port<u8> cmd(PortType::KeyboardControllerCommand);
    cmd.write(0xFE);
    return false;
}

// --- shutdown ---

static ShutdownCommand shutdownInstance;

ShutdownCommand::ShutdownCommand() : Command("shutdown", "Halt the system") {}

bool ShutdownCommand::execute(const char** args, usize argc,
                              FileNode*& cwd) {
    VgaTerminal::getTerminal().print("Shutting down...\n");
    return false;
}

// --- syscall ---

static SyscallCommand syscallInstance;

SyscallCommand::SyscallCommand() : Command("syscall", "Invoke a syscall by number") {}

bool SyscallCommand::execute(const char** args, usize argc,
                             FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("Usage: syscall <number> [arg1] [arg2] [arg3]\n");
        return true;
    }

    u32 number = strtou32(args[1]);

    // For write (syscall 0), treat args after fd as a string message.
    if (number == SyscallNumber::Write && argc >= 3) {
        u32 fd = strtou32(args[2]);

        // Build the message from remaining arguments.
        char msg[128];
        u32 pos = 0;

        for (usize i = 3; i < argc && pos < sizeof(msg) - 1; ++i) {
            if (i > 3 && pos < sizeof(msg) - 1) {
                msg[pos++] = ' ';
            }
            for (usize j = 0; args[i][j] != '\0' && pos < sizeof(msg) - 1; ++j) {
                msg[pos++] = args[i][j];
            }
        }
        msg[pos] = '\0';

        i32 result;
        asm volatile(
            "int $0x80"
            : "=a"(result)
            : "a"(number), "b"(fd), "c"((u32)msg), "d"(pos)
        );

        vga.print("Result: ");
        vga.print_dec(static_cast<u32>(result));
        vga.putchar('\n');
        return true;
    }

    u32 ebx = (argc >= 3) ? strtou32(args[2]) : 0;
    u32 ecx = (argc >= 4) ? strtou32(args[3]) : 0;
    u32 edx = (argc >= 5) ? strtou32(args[4]) : 0;

    i32 result;
    asm volatile(
        "int $0x80"
        : "=a"(result)
        : "a"(number), "b"(ebx), "c"(ecx), "d"(edx)
    );

    vga.print("Result: ");
    vga.print_dec(static_cast<u32>(result));
    vga.putchar('\n');

    return true;
}
