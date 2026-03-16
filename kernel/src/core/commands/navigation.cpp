/**
 * navigation.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/navigation.hpp"
#include "common/string.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

// --- ls ---

static LsCommand lsInstance;

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

// --- cd ---

static CdCommand cdInstance;

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

// --- pwd ---

static PwdCommand pwdInstance;

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
