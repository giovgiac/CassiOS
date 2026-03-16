/**
 * shell.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/shell.hpp"
#include "core/commands/command.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::hardware;

Shell::Shell()
    : vga(VgaTerminal::getTerminal()),
      cwd(filesystem::Filesystem::getFilesystem().getRoot()),
      length(0),
      cursor(0),
      prompt_x(0),
      prompt_y(0),
      running(false) {}

void Shell::run() {
    running = true;
    printPrompt();

    while (running) {
        asm volatile("hlt");
    }
}

void Shell::printPrompt() {
    prompt_x = vga.getCursorX();
    prompt_y = vga.getCursorY();
    vga.print("$ ");
    prompt_x = vga.getCursorX();
    prompt_y = vga.getCursorY();
}

void Shell::redrawLine() {
    vga.setCursor(prompt_x, prompt_y);

    for (u8 i = 0; i < length; ++i) {
        vga.putchar(buffer[i]);
    }

    // Clear any leftover characters from a previous longer line.
    vga.putchar(' ');

    // Position cursor at the editing point.
    vga.setCursor(prompt_x + cursor, prompt_y);
}

void Shell::execute() {
    buffer[length] = '\0';

    vga.putchar('\n');

    if (length == 0) {
        printPrompt();
        return;
    }

    // Split buffer into arguments by replacing spaces with null bytes.
    const char* args[SHELL_MAX_ARGS];
    usize argc = 0;
    bool in_word = false;

    for (u8 i = 0; i <= length; ++i) {
        if (buffer[i] == ' ' || buffer[i] == '\0') {
            buffer[i] = '\0';
            in_word = false;
        } else if (!in_word) {
            if (argc < SHELL_MAX_ARGS) {
                args[argc++] = &buffer[i];
            }
            in_word = true;
        }
    }

    Command* cmd = Command::find(args[0]);
    if (cmd) {
        running = cmd->execute(args, argc, cwd);
    } else {
        vga.print("Unknown command: ");
        vga.print(args[0]);
        vga.putchar('\n');
    }

    length = 0;
    cursor = 0;

    if (running) {
        printPrompt();
    }
}

void Shell::OnKeyDown(KeyCode key) {
    switch (key) {
    case KeyCode::Enter:
        execute();
        return;

    case KeyCode::Backspace:
        if (cursor > 0) {
            for (u8 i = cursor - 1; i < length - 1; ++i) {
                buffer[i] = buffer[i + 1];
            }
            --length;
            --cursor;
            redrawLine();
        }
        return;

    case KeyCode::Delete:
        if (cursor < length) {
            for (u8 i = cursor; i < length - 1; ++i) {
                buffer[i] = buffer[i + 1];
            }
            --length;
            redrawLine();
        }
        return;

    case KeyCode::LeftArrow:
        if (cursor > 0) {
            --cursor;
            vga.setCursor(prompt_x + cursor, prompt_y);
        }
        return;

    case KeyCode::RightArrow:
        if (cursor < length) {
            ++cursor;
            vga.setCursor(prompt_x + cursor, prompt_y);
        }
        return;

    default:
        break;
    }

    u8 ch = static_cast<u8>(key);
    if (ch >= 0x20 && ch <= 0x7E && length < SHELL_MAX_INPUT) {
        // Shift characters right to make room at cursor position.
        for (u8 i = length; i > cursor; --i) {
            buffer[i] = buffer[i - 1];
        }
        buffer[cursor] = static_cast<char>(key);
        ++length;
        ++cursor;
        redrawLine();
    }
}
