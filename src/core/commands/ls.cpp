/**
 * ls.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/ls.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static LsCommand instance;

LsCommand::LsCommand() : Command("ls", "List directory contents") {}

bool LsCommand::execute(const char** args, usize argc,
                        FileNode*& cwd) {
    Filesystem& fs = Filesystem::getFilesystem();
    VgaTerminal& vga = VgaTerminal::getTerminal();

    FileNode* target = cwd;
    if (argc > 1) {
        target = fs.resolve(args[1], cwd);
        if (target == nullptr) {
            vga.print("ls: no such directory: ");
            vga.print(args[1]);
            vga.putchar('\n');
            return true;
        }
    }

    if (target->type != FileNodeType::Directory) {
        vga.print(target->name);
        vga.putchar('\n');
        return true;
    }

    FileNode* child = target->children;
    while (child != nullptr) {
        vga.print(child->name);
        if (child->type == FileNodeType::Directory) {
            vga.putchar('/');
        }
        vga.putchar('\n');
        child = child->next;
    }

    return true;
}
