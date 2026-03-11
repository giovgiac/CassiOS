/**
 * help.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/help.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::hardware;

static HelpCommand instance;

HelpCommand::HelpCommand() : Command("help", "Show available commands") {}

bool HelpCommand::execute(const char** args, usize argc) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    vga.print("Available commands:\n");

    Command** cmds = Command::getRegistry();
    u8 count = Command::getCount();
    for (u8 i = 0; i < count; ++i) {
        vga.print("  ");
        vga.print(cmds[i]->getName());

        // Pad to 12 characters for alignment.
        usize len = 0;
        const char* n = cmds[i]->getName();
        while (n[len] != '\0') ++len;
        for (usize j = len; j < 12; ++j) {
            vga.putchar(' ');
        }

        vga.print("- ");
        vga.print(cmds[i]->getDescription());
        vga.putchar('\n');
    }

    return true;
}
