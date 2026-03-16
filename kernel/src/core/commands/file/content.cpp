/**
 * content.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/file/content.hpp"
#include "common/string.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

// --- cat ---

static CatCommand catInstance;

CatCommand::CatCommand() : Command("cat", "Print file contents") {}

bool CatCommand::execute(const char** args, usize argc,
                         FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 2) {
        vga.print("cat: usage: cat <filename>\n");
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

// --- write ---

static WriteCommand writeInstance;

WriteCommand::WriteCommand() : Command("write", "Write text to a file") {}

bool WriteCommand::execute(const char** args, usize argc,
                           FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    if (argc < 3) {
        vga.print("write: usage: write <filename> <text>\n");
        return true;
    }

    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* target = fs.resolve(args[1], cwd);
    if (target == nullptr) {
        vga.print("write: no such file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    if (target->type != FileNodeType::File) {
        vga.print("write: not a file: ");
        vga.print(args[1]);
        vga.putchar('\n');
        return true;
    }

    // Build the text buffer from args[2..argc-1] joined by spaces.
    u8 buf[256];
    usize pos = 0;
    for (usize i = 2; i < argc && pos < sizeof(buf); ++i) {
        if (i > 2 && pos < sizeof(buf)) {
            buf[pos++] = ' ';
        }
        for (usize j = 0; args[i][j] != '\0' && pos < sizeof(buf); ++j) {
            buf[pos++] = static_cast<u8>(args[i][j]);
        }
    }

    fs.write(target, buf, pos);
    return true;
}

// --- echo ---

static EchoCommand echoInstance;

EchoCommand::EchoCommand() : Command("echo", "Print text to screen") {}

bool EchoCommand::execute(const char** args, usize argc,
                          FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();

    for (usize i = 1; i < argc; ++i) {
        if (i > 1) vga.putchar(' ');
        vga.print(args[i]);
    }

    vga.putchar('\n');
    return true;
}
