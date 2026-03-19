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

#include <std/types.hpp>
#include <std/vga.hpp>

namespace cassio {

constexpr std::u8 SHELL_MAX_INPUT = 78;
constexpr std::u8 SHELL_MAX_ARGS = 16;
constexpr std::u32 SHELL_MAX_PATH = 64;

class Shell {
private:
    std::u32 kbdPid;
    std::vga::Vga vga;
    std::u32 vfsPid;

    char buffer[SHELL_MAX_INPUT + 1];  // +1 for null terminator in execute()
    std::u8 length;
    std::u8 cursor;
    std::u8 promptCol;
    std::u8 promptRow;

    char cwd[SHELL_MAX_PATH];

    // VGA helpers (use blocking send for ordering).
    void print(const char* str);
    void putchar(char ch);

    void printPrompt();
    void redrawLine();
    void execute();

    // Commands.
    void cmdHelp();
    void cmdClear();
    void cmdEcho(const char** args, std::u8 argc);
    void cmdMem();
    void cmdPs();
    void cmdUptime();
    void cmdReboot();
    void cmdShutdown();
    void cmdLs(const char** args, std::u8 argc);
    void cmdCd(const char** args, std::u8 argc);
    void cmdPwd();
    void cmdMkdir(const char** args, std::u8 argc);
    void cmdRmdir(const char** args, std::u8 argc);
    void cmdTouch(const char** args, std::u8 argc);
    void cmdRm(const char** args, std::u8 argc);
    void cmdCat(const char** args, std::u8 argc);
    void cmdWrite(const char** args, std::u8 argc);

public:
    Shell(std::u32 kbd, std::u32 vfs);
    void run();

    // Testable helpers.
    static std::u8 parseArgs(char* buf, std::u8 length, const char** args, std::u8 maxArgs);
    static void resolvePath(const char* cwd, const char* input,
                            char* out, std::u32 maxLen);
    static void parentDir(char* path);
};

} // cassio

#endif // USERSPACE_SHELL_SHELL_HPP_
