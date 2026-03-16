/**
 * management.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/file/management.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

// --- mkdir ---

static MkdirCommand mkdirInstance;

MkdirCommand::MkdirCommand() : Command("mkdir", "Create a directory") {}

bool MkdirCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("mkdir: usage: mkdir <path>\n");
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

// --- rmdir ---

static RmdirCommand rmdirInstance;

RmdirCommand::RmdirCommand() : Command("rmdir", "Remove an empty directory") {}

bool RmdirCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("rmdir: usage: rmdir <path>\n");
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

// --- touch ---

static TouchCommand touchInstance;

TouchCommand::TouchCommand() : Command("touch", "Create an empty file") {}

bool TouchCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("touch: usage: touch <path>\n");
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

// --- rm ---

static RmCommand rmInstance;

RmCommand::RmCommand() : Command("rm", "Remove a file") {}

bool RmCommand::execute(const char** args, usize argc,
                        FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("rm: usage: rm <path>\n");
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
