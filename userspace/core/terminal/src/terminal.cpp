/**
 * terminal.cpp -- Graphics terminal
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <terminal.hpp>

#include <std/mem.hpp>
#include <std/os.hpp>

using namespace cassio;
using namespace std;
using namespace std::gfx;

Terminal::Terminal(display::Display& display, u32 screenWidth, u32 screenHeight)
    : display(display), cols(screenWidth / FONT_WIDTH), rows(screenHeight / FONT_HEIGHT), x(0),
      y(0), fg(0x00AAAAAA), bg(0x00000000), cells(nullptr), pendingScroll(0) {
    u32 gridSize = cols * rows;
    if (gridSize > 0) {
        cells = static_cast<char*>(os::sbrk(gridSize));
        mem::set(cells, ' ', gridSize);
    }
}

char Terminal::cellAt(u32 col, u32 row) const {
    if (!cells || col >= cols || row >= rows) {
        return ' ';
    }
    return cells[row * cols + col];
}

void Terminal::setCellAt(u32 col, u32 row, char ch) {
    if (cells && col < cols && row < rows) {
        cells[row * cols + col] = ch;
    }
}

void Terminal::flushScroll() {
    if (pendingScroll > 0) {
        display.scroll(pendingScroll * FONT_HEIGHT, bg);

        // Shift the character grid up and clear the new bottom rows.
        if (cells) {
            u32 scrollRows = pendingScroll;
            if (scrollRows >= rows) {
                mem::set(cells, ' ', cols * rows);
            } else {
                mem::move(cells, cells + scrollRows * cols, (rows - scrollRows) * cols);
                mem::set(cells + (rows - scrollRows) * cols, ' ', scrollRows * cols);
            }
        }

        pendingScroll = 0;
    }
}

void Terminal::drawGlyph(char ch, u32 col, u32 row) {
    flushScroll();
    setCellAt(col, row, ch);
    display.drawChar(col * FONT_WIDTH, row * FONT_HEIGHT, ch, fg, bg);
}

void Terminal::clearCell(u32 col, u32 row) {
    flushScroll();
    setCellAt(col, row, ' ');
    display.fillRect(col * FONT_WIDTH, row * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, bg);
}

void Terminal::putchar(char ch) {
    if (cols == 0 || rows == 0) {
        return;
    }

    switch (ch) {
    case '\n':
        y += 1;
        x = 0;
        break;

    case '\b':
        if (x > 0) {
            x -= 1;
        } else if (y > 0) {
            y -= 1;
            x = static_cast<u8>(cols - 1);
        }
        clearCell(x, y);
        break;

    case 0x7F: // DEL
        clearCell(x, y);
        break;

    case '\t': {
        u8 next = (x + 8) & ~7;
        if (next >= cols) {
            next = static_cast<u8>(cols);
        }
        while (x < next) {
            clearCell(x, y);
            x += 1;
        }
        break;
    }

    default:
        drawGlyph(ch, x, y);
        x += 1;
        break;
    }

    if (x >= cols) {
        y += 1;
        x = 0;
    }

    if (y >= rows) {
        pendingScroll += y - rows + 1;
        y = static_cast<u8>(rows - 1);
    }
}

void Terminal::clear() {
    pendingScroll = 0;
    if (cells) {
        mem::set(cells, ' ', cols * rows);
    }
    display.fillRect(0, 0, cols * FONT_WIDTH, rows * FONT_HEIGHT, bg);
    x = 0;
    y = 0;
}

void Terminal::drawCursor() {
    flushScroll();
    // Draw the character under the cursor with inverted colors.
    char ch = cellAt(x, y);
    display.drawChar(x * FONT_WIDTH, y * FONT_HEIGHT, ch, bg, fg);
}

void Terminal::eraseCursor() {
    flushScroll();
    // Redraw the character under the cursor with normal colors.
    char ch = cellAt(x, y);
    display.drawChar(x * FONT_WIDTH, y * FONT_HEIGHT, ch, fg, bg);
}

void Terminal::setCursor(u8 col, u8 row) {
    x = col;
    y = row;
}

u8 Terminal::getCursorX() const {
    return x;
}
u8 Terminal::getCursorY() const {
    return y;
}
u32 Terminal::getCols() const {
    return cols;
}
u32 Terminal::getRows() const {
    return rows;
}
