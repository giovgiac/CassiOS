/**
 * navigation.cpp -- Navigation shell commands
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <std/str.hpp>
#include <vfs.hpp>

using namespace cassio;
using namespace std;

void Shell::cmdLs(const char** args, u8 argc) {
    char path[SHELL_MAX_PATH];
    if (argc > 1) {
        resolvePath(cwd, args[1], path, SHELL_MAX_PATH);
    } else {
        str::copy(path, cwd, SHELL_MAX_PATH);
    }

    if (Vfs::stat(vfsPid, path) != 2) {
        print("ls: no such directory: ");
        print(path);
        putchar('\n');
        return;
    }

    char name[32];
    for (u32 i = 0; ; ++i) {
        if (!Vfs::list(vfsPid, path, i, name, sizeof(name))) {
            break;
        }
        print(name);
        putchar('\n');
    }
}

void Shell::cmdCd(const char** args, u8 argc) {
    if (argc < 2) {
        cwd[0] = '/';
        cwd[1] = '\0';
        return;
    }

    if (str::eq(args[1], "..")) {
        parentDir(cwd);
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);

    if (Vfs::stat(vfsPid, path) != 2) {
        print("cd: no such directory: ");
        print(args[1]);
        putchar('\n');
        return;
    }

    str::copy(cwd, path, SHELL_MAX_PATH);
}

void Shell::cmdPwd() {
    print(cwd);
    putchar('\n');
}
