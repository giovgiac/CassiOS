/**
 * syscall_cmd.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/syscall_cmd.hpp"
#include "common/string.hpp"
#include "core/syscall.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

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
