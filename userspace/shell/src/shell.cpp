/**
 * shell.cpp -- Userspace shell implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <std/str.hpp>
#include <std/mem.hpp>
#include <std/fmt.hpp>
#include <keycode.hpp>
#include <std/ipc.hpp>

using namespace cassio;
using namespace std;

Shell::Shell(u32 kbd, u32 vfs)
    : kbdPid(kbd), vfsPid(vfs),
      length(0), cursor(0), promptCol(0), promptRow(0) {
    cwd[0] = '/';
    cwd[1] = '\0';
    for (u8 i = 0; i < SHELL_MAX_INPUT; ++i) {
        buffer[i] = '\0';
    }
}

// --- VGA helpers ---
// Use fire-and-forget notify for output (no round-trip, fast).
// Only printPrompt and redrawLine use blocking send (need cursor position
// or ordering with setCursor).

void Shell::print(const char* str) {
    vga.write(str);
}

void Shell::putchar(char ch) {
    vga.putchar(ch);
}

// --- Prompt and line editing ---

void Shell::printPrompt() {
    print("$ ");
    vga.getCursor(promptCol, promptRow);
}

void Shell::redrawLine() {
    vga.setCursor(promptCol, promptRow);

    // Write the entire buffer.
    if (length > 0) {
        vga.write(buffer, length);
    }

    // Clear any leftover character from a previous longer line.
    putchar(' ');

    // Position cursor at the editing point.
    vga.setCursor(promptCol + cursor, promptRow);
}

void Shell::execute() {
    buffer[length] = '\0';
    putchar('\n');

    if (length == 0) {
        printPrompt();
        return;
    }

    // Make a copy for parsing (parseArgs modifies the buffer).
    char copy[SHELL_MAX_INPUT];
    mem::copy(copy, buffer, length + 1);

    const char* args[SHELL_MAX_ARGS];
    u8 argc = parseArgs(copy, length, args, SHELL_MAX_ARGS);

    if (argc == 0) {
        printPrompt();
        return;
    }

    if (str::eq(args[0], "help"))          cmdHelp();
    else if (str::eq(args[0], "clear"))    cmdClear();
    else if (str::eq(args[0], "echo"))     cmdEcho(args, argc);
    else if (str::eq(args[0], "mem"))      cmdMem();
    else if (str::eq(args[0], "ps"))       cmdPs();
    else if (str::eq(args[0], "uptime"))   cmdUptime();
    else if (str::eq(args[0], "reboot"))   cmdReboot();
    else if (str::eq(args[0], "shutdown")) cmdShutdown();
    else if (str::eq(args[0], "ls"))       cmdLs(args, argc);
    else if (str::eq(args[0], "cd"))       cmdCd(args, argc);
    else if (str::eq(args[0], "pwd"))      cmdPwd();
    else if (str::eq(args[0], "mkdir"))    cmdMkdir(args, argc);
    else if (str::eq(args[0], "rmdir"))    cmdRmdir(args, argc);
    else if (str::eq(args[0], "touch"))    cmdTouch(args, argc);
    else if (str::eq(args[0], "rm"))       cmdRm(args, argc);
    else if (str::eq(args[0], "cat"))      cmdCat(args, argc);
    else if (str::eq(args[0], "write"))    cmdWrite(args, argc);
    else {
        print("Unknown command: ");
        print(args[0]);
        putchar('\n');
    }

    length = 0;
    cursor = 0;
    printPrompt();
}

void Shell::run() {
    printPrompt();

    while (true) {
        ipc::Message msg = {};
        msg.type = ipc::MessageType::KbdRead;
        ipc::send(kbdPid, &msg);

        u8 key = static_cast<u8>(msg.arg1);
        if (key == 0) continue;

        switch (static_cast<KeyCode>(key)) {
        case KeyCode::Enter:
            execute();
            continue;

        case KeyCode::Backspace:
            if (cursor > 0) {
                for (u8 i = cursor - 1; i < length - 1; ++i) {
                    buffer[i] = buffer[i + 1];
                }
                --length;
                --cursor;
                if (cursor == length) {
                    // Deleting at end: backspace, clear, backspace.
                    putchar('\b');
                    putchar(' ');
                    putchar('\b');
                } else {
                    redrawLine();
                }
            }
            continue;

        case KeyCode::Delete:
            if (cursor < length) {
                for (u8 i = cursor; i < length - 1; ++i) {
                    buffer[i] = buffer[i + 1];
                }
                --length;
                redrawLine();
            }
            continue;

        case KeyCode::LeftArrow:
            if (cursor > 0) {
                --cursor;
                vga.setCursor(promptCol + cursor, promptRow);
            }
            continue;

        case KeyCode::RightArrow:
            if (cursor < length) {
                ++cursor;
                vga.setCursor(promptCol + cursor, promptRow);
            }
            continue;

        default:
            break;
        }

        if (key >= 0x20 && key <= 0x7E && length < SHELL_MAX_INPUT) {
            for (u8 i = length; i > cursor; --i) {
                buffer[i] = buffer[i - 1];
            }
            buffer[cursor] = static_cast<char>(key);
            ++length;
            ++cursor;
            if (cursor == length) {
                // Appended at end: just print the one character.
                putchar(static_cast<char>(key));
            } else {
                redrawLine();
            }
        }
    }
}

// --- Static helpers ---

u8 Shell::parseArgs(char* buf, u8 len, const char** args, u8 maxArgs) {
    u8 argc = 0;
    bool in_word = false;

    for (u8 i = 0; i <= len; ++i) {
        if (buf[i] == ' ' || buf[i] == '\0') {
            buf[i] = '\0';
            in_word = false;
        } else if (!in_word) {
            if (argc < maxArgs) {
                args[argc++] = &buf[i];
            }
            in_word = true;
        }
    }

    return argc;
}

void Shell::resolvePath(const char* cwdPath, const char* input,
                        char* out, u32 maxLen) {
    if (input[0] == '/') {
        str::copy(out, input, maxLen);
        return;
    }

    u32 cwdLen = str::len(cwdPath);
    u32 pos = 0;

    // Copy cwd.
    for (u32 i = 0; i < cwdLen && pos < maxLen - 1; ++i) {
        out[pos++] = cwdPath[i];
    }

    // Add separator if cwd is not "/".
    if (cwdLen > 1 && pos < maxLen - 1) {
        out[pos++] = '/';
    }

    // Copy input.
    for (u32 i = 0; input[i] != '\0' && pos < maxLen - 1; ++i) {
        out[pos++] = input[i];
    }
    out[pos] = '\0';
}

void Shell::parentDir(char* path) {
    u32 len = str::len(path);
    if (len <= 1) return;

    u32 last = len - 1;
    while (last > 0 && path[last] != '/') {
        --last;
    }

    if (last == 0) {
        path[1] = '\0';
    } else {
        path[last] = '\0';
    }
}

