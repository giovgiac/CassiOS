/**
 * terminal.hpp -- Graphics terminal
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Maintains a character grid, renders glyphs via the display service.
 * Handles control characters: newline, backspace, delete, tab.
 *
 */

#ifndef TERMINAL_HPP_
#define TERMINAL_HPP_

#include <std/display.hpp>
#include <std/gfx.hpp>
#include <std/types.hpp>

namespace cassio {

class Terminal {
public:
    Terminal(std::display::Display& display, std::u32 screenWidth, std::u32 screenHeight);

    void putchar(char ch);
    void clear();
    void setCursor(std::u8 col, std::u8 row);
    std::u8 getCursorX() const;
    std::u8 getCursorY() const;
    void drawCursor();
    void eraseCursor();

    std::u32 getCols() const;
    std::u32 getRows() const;

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

private:
    std::display::Display& display;
    std::u32 cols;
    std::u32 rows;
    std::u8 x;
    std::u8 y;
    std::gfx::Color fg;
    std::gfx::Color bg;

    char* cells;            // Character grid (cols * rows), heap-allocated.
    std::u32 pendingScroll; // Accumulated scroll rows (flushed before next draw).

    char cellAt(std::u32 col, std::u32 row) const;
    void setCellAt(std::u32 col, std::u32 row, char ch);
    void flushScroll();
    void drawGlyph(char ch, std::u32 col, std::u32 row);
    void clearCell(std::u32 col, std::u32 row);
};

} // namespace cassio

#endif // TERMINAL_HPP_
