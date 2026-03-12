/**
 * cp.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/cp.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static CpCommand instance;

CpCommand::CpCommand() : Command("cp", "Copy a file") {}

bool CpCommand::execute(const char** args, usize argc,
                        FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 3) {
        vga.print("cp: usage: cp <source> <destination>\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* source = fs.resolve(args[1], cwd);
    if (source == nullptr) {
        vga.print("cp: no such file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (source->type != FileNodeType::File) {
        vga.print("cp: not a file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (fs.copy(source, args[2], cwd) == nullptr) {
        vga.print("cp: cannot copy to: ");
        vga.print(args[2]);
        vga.putchar('\n');
    }

    return true;
}
