/**
 * write.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/write.hpp"
#include "common/string.hpp"
#include "filesystem/filesystem.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static WriteCommand instance;

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

    // Compute total text length (args[2..argc-1] joined by spaces).
    usize total = 0;
    for (usize i = 2; i < argc; ++i) {
        if (i > 2) ++total; // space separator
        total += strlen(args[i]);
    }

    // Build the text buffer.
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
