/**
 * touch.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/touch.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static TouchCommand instance;

TouchCommand::TouchCommand() : Command("touch", "Create an empty file") {}

bool TouchCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("touch: missing operand\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* file = fs.createFile(args[1], cwd);
    if (file == nullptr) {
        vga.print("touch: cannot create file: ");
        vga.print(args[1]);
        vga.putchar('\n');
    }

    return true;
}
