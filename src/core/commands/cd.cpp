/**
 * cd.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/cd.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static CdCommand instance;

CdCommand::CdCommand() : Command("cd", "Change working directory") {}

bool CdCommand::execute(const char** args, usize argc,
                        FileNode*& cwd) {
    Filesystem& fs = Filesystem::getFilesystem();
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        cwd = fs.getRoot();
        return true;
    }

    FileNode* target = fs.resolve(args[1], cwd);
    if (target == nullptr) {
        vga.print("cd: no such directory: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (target->type != FileNodeType::Directory) {
        vga.print("cd: not a directory: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    cwd = target;
    return true;
}
