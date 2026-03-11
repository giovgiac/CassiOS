/**
 * mkdir.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/mkdir.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static MkdirCommand instance;

MkdirCommand::MkdirCommand() : Command("mkdir", "Create a directory") {}

bool MkdirCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("mkdir: missing operand\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* dir = fs.createDirectory(args[1], cwd);
    if (dir == nullptr) {
        vga.print("mkdir: cannot create directory: ");
        vga.print(args[1]);
        vga.putchar('\n');
    }

    return true;
}
