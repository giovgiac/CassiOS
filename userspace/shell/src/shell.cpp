/**
 * shell.cpp -- Userspace shell implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <string.hpp>
#include <keycode.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <vga.hpp>
#include <vfs.hpp>
#include <system.hpp>

using namespace cassio;

Shell::Shell(u32 kbd, u32 vga, u32 vfs)
    : kbdPid(kbd), vgaPid(vga), vfsPid(vfs),
      length(0), cursor(0), promptCol(0), promptRow(0) {
    cwd[0] = '/';
    cwd[1] = '\0';
    for (u8 i = 0; i < SHELL_MAX_INPUT; ++i) {
        buffer[i] = '\0';
    }
}

// --- VGA helpers (blocking send for ordering) ---

void Shell::print(const char* str) {
    // Use blocking send (not notify) for correct ordering with setCursor.
    while (*str != '\0') {
        Message msg = {};
        msg.type = MessageType::VgaWrite;
        char* data = reinterpret_cast<char*>(&msg.arg1);
        u32 i = 0;
        while (i < 20 && str[i] != '\0') {
            data[i] = str[i];
            ++i;
        }
        if (i < 20) data[i] = '\0';
        IPC::send(vgaPid, &msg);
        str += i;
    }
}

void Shell::putchar(char ch) {
    Message msg = {};
    msg.type = MessageType::VgaPutchar;
    msg.arg1 = static_cast<u8>(ch);
    IPC::send(vgaPid, &msg);
}

void Shell::printDec(u32 val) {
    char tmp[12];
    u32 pos = 0;

    if (val == 0) {
        print("0");
        return;
    }

    while (val > 0 && pos < 11) {
        tmp[pos++] = '0' + (val % 10);
        val /= 10;
    }

    char buf[12];
    u32 i = 0;
    while (pos > 0) {
        buf[i++] = tmp[--pos];
    }
    buf[i] = '\0';
    print(buf);
}

// --- Prompt and line editing ---

void Shell::printPrompt() {
    // Single blocking send for "$ "; read cursor position from reply.
    Message msg = {};
    msg.type = MessageType::VgaWrite;
    char* data = reinterpret_cast<char*>(&msg.arg1);
    data[0] = '$';
    data[1] = ' ';
    data[2] = '\0';
    IPC::send(vgaPid, &msg);
    promptCol = static_cast<u8>(msg.arg1);
    promptRow = static_cast<u8>(msg.arg2);
}

void Shell::redrawLine() {
    Vga::setCursor(vgaPid, promptCol, promptRow);

    // Batch characters into 20-char VgaWrite chunks.
    u8 pos = 0;
    while (pos < length) {
        Message msg = {};
        msg.type = MessageType::VgaWrite;
        char* data = reinterpret_cast<char*>(&msg.arg1);
        u8 chunk = 0;
        while (chunk < 20 && pos + chunk < length) {
            data[chunk] = buffer[pos + chunk];
            ++chunk;
        }
        if (chunk < 20) data[chunk] = '\0';
        IPC::send(vgaPid, &msg);
        pos += chunk;
    }

    // Clear any leftover character from a previous longer line.
    putchar(' ');

    // Position cursor at the editing point.
    Vga::setCursor(vgaPid, promptCol + cursor, promptRow);
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
    for (u8 i = 0; i <= length; ++i) {
        copy[i] = buffer[i];
    }

    const char* args[SHELL_MAX_ARGS];
    u8 argc = parseArgs(copy, length, args, SHELL_MAX_ARGS);

    if (argc == 0) {
        printPrompt();
        return;
    }

    if (streq(args[0], "help"))          cmdHelp();
    else if (streq(args[0], "clear"))    cmdClear();
    else if (streq(args[0], "echo"))     cmdEcho(args, argc);
    else if (streq(args[0], "uptime"))   cmdUptime();
    else if (streq(args[0], "reboot"))   cmdReboot();
    else if (streq(args[0], "shutdown")) cmdShutdown();
    else if (streq(args[0], "ls"))       cmdLs(args, argc);
    else if (streq(args[0], "cd"))       cmdCd(args, argc);
    else if (streq(args[0], "pwd"))      cmdPwd();
    else if (streq(args[0], "mkdir"))    cmdMkdir(args, argc);
    else if (streq(args[0], "rmdir"))    cmdRmdir(args, argc);
    else if (streq(args[0], "touch"))    cmdTouch(args, argc);
    else if (streq(args[0], "rm"))       cmdRm(args, argc);
    else if (streq(args[0], "cat"))      cmdCat(args, argc);
    else if (streq(args[0], "write"))    cmdWrite(args, argc);
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
        Message msg = {};
        msg.type = MessageType::KbdRead;
        IPC::send(kbdPid, &msg);

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
                Vga::setCursor(vgaPid, promptCol + cursor, promptRow);
            }
            continue;

        case KeyCode::RightArrow:
            if (cursor < length) {
                ++cursor;
                Vga::setCursor(vgaPid, promptCol + cursor, promptRow);
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
        strcpy(out, input, maxLen);
        return;
    }

    u32 cwdLen = strlen(cwdPath);
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
    u32 len = strlen(path);
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

// --- Commands ---

void Shell::cmdHelp() {
    print("Available commands:\n");
    print("  help        - Show this help\n");
    print("  clear       - Clear the screen\n");
    print("  echo        - Print text\n");
    print("  uptime      - Show time since boot\n");
    print("  reboot      - Reboot the system\n");
    print("  shutdown    - Halt the system\n");
    print("  ls          - List directory\n");
    print("  cd          - Change directory\n");
    print("  pwd         - Print working directory\n");
    print("  mkdir       - Create directory\n");
    print("  rmdir       - Remove directory\n");
    print("  touch       - Create empty file\n");
    print("  rm          - Remove file\n");
    print("  cat         - Print file contents\n");
    print("  write       - Write text to file\n");
}

void Shell::cmdClear() {
    Vga::clear(vgaPid);
}

void Shell::cmdEcho(const char** args, u8 argc) {
    for (u8 i = 1; i < argc; ++i) {
        if (i > 1) putchar(' ');
        print(args[i]);
    }
    putchar('\n');
}

void Shell::cmdUptime() {
    i32 ticks = System::uptime();
    u32 total_ms = (static_cast<u32>(ticks) * 1000) / 100;
    u32 seconds = total_ms / 1000;
    u32 ms = total_ms % 1000;

    print("Up ");
    printDec(seconds);
    putchar('.');
    if (ms < 100) putchar('0');
    if (ms < 10) putchar('0');
    printDec(ms);
    print("s\n");
}

void Shell::cmdReboot() {
    print("Rebooting...\n");
    System::reboot();
}

void Shell::cmdShutdown() {
    print("Shutting down...\n");
    System::shutdown();
}

void Shell::cmdLs(const char** args, u8 argc) {
    char path[SHELL_MAX_PATH];
    if (argc > 1) {
        resolvePath(cwd, args[1], path, SHELL_MAX_PATH);
    } else {
        strcpy(path, cwd, SHELL_MAX_PATH);
    }

    char name[20];
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

    if (streq(args[1], "..")) {
        parentDir(cwd);
        return;
    }

    char path[SHELL_MAX_PATH];
    resolvePath(cwd, args[1], path, SHELL_MAX_PATH);
    strcpy(cwd, path, SHELL_MAX_PATH);
}

void Shell::cmdPwd() {
    print(cwd);
    putchar('\n');
}

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
