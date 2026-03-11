/**
 * rmdir.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/rmdir.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static RmdirCommand instance;

RmdirCommand::RmdirCommand() : Command("rmdir", "Remove an empty directory") {}

bool RmdirCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("rmdir: missing operand\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* target = fs.resolve(args[1], cwd);
    if (target == nullptr) {
        vga.print("rmdir: no such directory: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (target->type != FileNodeType::Directory) {
        vga.print("rmdir: not a directory: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (!fs.remove(target)) {
        vga.print("rmdir: directory not empty: ");
        vga.print(args[1]);
        vga.putchar('\n');
    }

    return true;
}
