/**
 * cat.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/cat.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static CatCommand instance;

CatCommand::CatCommand() : Command("cat", "Print file contents") {}

bool CatCommand::execute(const char** args, usize argc,
                         FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("cat: missing operand\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* target = fs.resolve(args[1], cwd);
    if (target == nullptr) {
        vga.print("cat: no such file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (target->type != FileNodeType::File) {
        vga.print("cat: not a file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    for (usize i = 0; i < target->size; ++i) {
        vga.putchar(static_cast<char>(target->data[i]));
    }

    if (target->size > 0) {
        vga.putchar('\n');
    }

    return true;
}
