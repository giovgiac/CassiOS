/**
 * terminal.cpp -- Graphics terminal
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <terminal.hpp>

using namespace cassio;
using namespace std;
using namespace std::gfx;

Terminal::Terminal(display::Display& display, u32 screenWidth, u32 screenHeight)
    : display(display), cols(screenWidth / FONT_WIDTH), rows(screenHeight / FONT_HEIGHT), x(0),
      y(0), fg(0x00AAAAAA), bg(0x00000000) {}

void Terminal::drawGlyph(char ch, u32 col, u32 row) {
    // Render glyph into a small local buffer, then blit to display.
    u32 pixels[FONT_WIDTH * FONT_HEIGHT];
    PixelBuffer buf(pixels, FONT_WIDTH, FONT_HEIGHT, FONT_WIDTH * sizeof(u32));
    buf.drawChar(0, 0, ch, fg, bg);
    display.blit(col * FONT_WIDTH, row * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, pixels);
}

void Terminal::clearCell(u32 col, u32 row) {
    display.fillRect(col * FONT_WIDTH, row * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, bg);
}

void Terminal::scrollUp() {
    display.scroll(FONT_HEIGHT, bg);
    y = static_cast<u8>(rows - 1);
}

void Terminal::putchar(char ch) {
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
        scrollUp();
    }
}

void Terminal::clear() {
    display.fillRect(0, 0, cols * FONT_WIDTH, rows * FONT_HEIGHT, bg);
    x = 0;
    y = 0;
}

void Terminal::drawCursor() {
    // Underline cursor: 8x2 bar at the bottom of the character cell.
    u32 px = x * FONT_WIDTH;
    u32 py = y * FONT_HEIGHT + FONT_HEIGHT - 2;
    display.fillRect(px, py, FONT_WIDTH, 2, fg);
}

void Terminal::eraseCursor() {
    u32 px = x * FONT_WIDTH;
    u32 py = y * FONT_HEIGHT + FONT_HEIGHT - 2;
    display.fillRect(px, py, FONT_WIDTH, 2, bg);
}

void Terminal::setCursor(u8 col, u8 row) {
    eraseCursor();
    x = col;
    y = row;
    drawCursor();
}

u8 Terminal::getCursorX() const { return x; }
u8 Terminal::getCursorY() const { return y; }
u32 Terminal::getCols() const { return cols; }
u32 Terminal::getRows() const { return rows; }
