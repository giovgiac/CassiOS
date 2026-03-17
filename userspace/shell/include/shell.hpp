/**
 * shell.hpp -- Userspace shell
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_SHELL_SHELL_HPP_
#define USERSPACE_SHELL_SHELL_HPP_

#include <types.hpp>

namespace cassio {

constexpr u8 SHELL_MAX_INPUT = 78;
constexpr u8 SHELL_MAX_ARGS = 16;
constexpr u32 SHELL_MAX_PATH = 64;

class Shell {
private:
    u32 kbdPid;
    u32 vgaPid;
    u32 vfsPid;

    char buffer[SHELL_MAX_INPUT];
    u8 length;
    u8 cursor;
    u8 promptCol;
    u8 promptRow;

    char cwd[SHELL_MAX_PATH];

    // VGA helpers (use blocking send for ordering).
    void print(const char* str);
    void putchar(char ch);
    void printDec(u32 val);

    void printPrompt();
    void redrawLine();
    void execute();

    // Commands.
    void cmdHelp();
    void cmdClear();
    void cmdEcho(const char** args, u8 argc);
    void cmdMem();
    void cmdUptime();
    void cmdReboot();
    void cmdShutdown();
    void cmdLs(const char** args, u8 argc);
    void cmdCd(const char** args, u8 argc);
    void cmdPwd();
    void cmdMkdir(const char** args, u8 argc);
    void cmdRmdir(const char** args, u8 argc);
    void cmdTouch(const char** args, u8 argc);
    void cmdRm(const char** args, u8 argc);
    void cmdCat(const char** args, u8 argc);
    void cmdWrite(const char** args, u8 argc);

public:
    Shell(u32 kbd, u32 vga, u32 vfs);
    void run();

    // Testable helpers.
    static u8 parseArgs(char* buf, u8 length, const char** args, u8 maxArgs);
    static void resolvePath(const char* cwd, const char* input,
                            char* out, u32 maxLen);
    static void parentDir(char* path);
};

} // cassio

#endif // USERSPACE_SHELL_SHELL_HPP_
