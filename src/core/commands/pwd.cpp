/**
 * pwd.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/pwd.hpp"
#include "common/string.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static PwdCommand instance;

PwdCommand::PwdCommand() : Command("pwd", "Print working directory") {}

bool PwdCommand::execute(const char** args, usize argc,
                         FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    Filesystem& fs = Filesystem::getFilesystem();

    if (cwd == fs.getRoot()) {
        vga.print("/\n");
        return true;
    }

    // Build path by collecting ancestors up to root.
    // Max depth is bounded by available nodes; 32 levels is generous.
    constexpr usize MAX_DEPTH = 32;
    FileNode* stack[MAX_DEPTH];
    usize depth = 0;

    FileNode* node = cwd;
    while (node != fs.getRoot() && depth < MAX_DEPTH) {
        stack[depth++] = node;
        node = node->parent;
    }

    // Print from root down.
    for (usize i = depth; i > 0; --i) {
        vga.putchar('/');
        vga.print(stack[i - 1]->name);
    }
    vga.putchar('\n');

    return true;
}
