/**
 * echo.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/echo.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static EchoCommand instance;

EchoCommand::EchoCommand() : Command("echo", "Print text to screen") {}

bool EchoCommand::execute(const char** args, usize argc,
                          FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    for (usize i = 1; i < argc; ++i) {
        if (i > 1) vga.putchar(' ');
        vga.print(args[i]);
    }

    vga.putchar('\n');
    return true;
}
