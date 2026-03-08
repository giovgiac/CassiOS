/**
 * shell.cpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/shell.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::drivers;
using namespace cassio::hardware;

Shell::Shell()
    : vga(VgaTerminal::getTerminal()),
      length(0),
      cursor(0),
      prompt_x(0),
      prompt_y(0) {
    printPrompt();
}

bool Shell::streq(const char* a, const char* b) {
    u32 i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == b[i];
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

    if (streq(buffer, "shutdown")) {
        vga.print("Shutting down...\n");
        asm volatile("cli");
        asm volatile("hlt");
        return;
    }

    if (streq(buffer, "help")) {
        vga.print("Available commands:\n");
        vga.print("  shutdown  - Halt the system\n");
        vga.print("  help      - Show this message\n");
        vga.print("  clear     - Clear the screen\n");
    } else if (streq(buffer, "clear")) {
        vga.clear();
    } else {
        vga.print("Unknown command: ");
        vga.print(buffer);
        vga.putchar('\n');
    }

    length = 0;
    cursor = 0;
    printPrompt();
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
