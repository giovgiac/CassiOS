/**
 * mv.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/mv.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static MvCommand instance;

MvCommand::MvCommand() : Command("mv", "Move or rename a file or directory") {}

bool MvCommand::execute(const char** args, usize argc,
                        FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 3) {
        vga.print("mv: usage: mv <source> <destination>\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* source = fs.resolve(args[1], cwd);
    if (source == nullptr) {
        vga.print("mv: no such file or directory: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (!fs.move(source, args[2], cwd)) {
        vga.print("mv: cannot move to: ");
        vga.print(args[2]);
        vga.putchar('\n');
    }

    return true;
}
