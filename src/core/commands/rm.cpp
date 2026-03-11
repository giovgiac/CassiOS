/**
 * rm.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/rm.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static RmCommand instance;

RmCommand::RmCommand() : Command("rm", "Remove a file") {}

bool RmCommand::execute(const char** args, usize argc,
                        FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("rm: missing operand\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* target = fs.resolve(args[1], cwd);
    if (target == nullptr) {
        vga.print("rm: no such file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (target->type != FileNodeType::File) {
        vga.print("rm: not a file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    fs.remove(target);
    return true;
}
