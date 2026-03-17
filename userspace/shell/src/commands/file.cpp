/**
 * file.cpp -- File management shell commands
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <string.hpp>
#include <vfs.hpp>

using namespace cassio;

void Shell::cmdMkdir(const char** args, u8 argc) {
    if (argc < 2) {
        print("mkdir: usage: mkdir <path>\n");
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    u32 result = Vfs::mkdir(vfsPid, path);
    if (result != 0) {
        print("mkdir: cannot create directory: ");
        print(args[1]);
        putchar('\n');
    }
}

void Shell::cmdRmdir(const char** args, u8 argc) {
    if (argc < 2) {
        print("rmdir: usage: rmdir <path>\n");
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    u32 result = Vfs::remove(vfsPid, path);
    if (result != 0) {
        print("rmdir: cannot remove: ");
        print(args[1]);
        putchar('\n');
    }
}

void Shell::cmdTouch(const char** args, u8 argc) {
    if (argc < 2) {
        print("touch: usage: touch <path>\n");
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    u32 handle = Vfs::open(vfsPid, path);
    if (handle == 0) {
        print("touch: cannot create file: ");
        print(args[1]);
        putchar('\n');
    }
}

void Shell::cmdRm(const char** args, u8 argc) {
    if (argc < 2) {
        print("rm: usage: rm <path>\n");
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    u32 result = Vfs::remove(vfsPid, path);
    if (result != 0) {
        print("rm: cannot remove: ");
        print(args[1]);
        putchar('\n');
    }
}

void Shell::cmdCat(const char** args, u8 argc) {
    if (argc < 2) {
        print("cat: usage: cat <filename>\n");
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    u32 handle = Vfs::open(vfsPid, path);
    if (handle == 0) {
        print("cat: no such file: ");
        print(args[1]);
        putchar('\n');
        return;
    }

    u8 buf[16];
    u32 offset = 0;
    while (true) {
        i32 n = Vfs::read(vfsPid, handle, offset, buf, sizeof(buf));
        if (n <= 0) break;
        for (i32 i = 0; i < n; ++i) {
            putchar(static_cast<char>(buf[i]));
        }
        offset += static_cast<u32>(n);
    }
    putchar('\n');
}

void Shell::cmdWrite(const char** args, u8 argc) {
    if (argc < 3) {
        print("write: usage: write <filename> <text>\n");
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    u32 handle = Vfs::open(vfsPid, path);
    if (handle == 0) {
        print("write: no such file: ");
        print(args[1]);
        putchar('\n');
        return;
    }

    // Build the text from args[2..argc-1] joined by spaces.
    u8 text[64];
    u32 pos = 0;
    for (u8 i = 2; i < argc && pos < sizeof(text); ++i) {
        if (i > 2 && pos < sizeof(text)) {
            text[pos++] = ' ';
        }
        for (u32 j = 0; args[i][j] != '\0' && pos < sizeof(text); ++j) {
            text[pos++] = static_cast<u8>(args[i][j]);
        }
    }

    Vfs::write(vfsPid, handle, text, pos);
}
