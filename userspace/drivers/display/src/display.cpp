/**
 * display.cpp -- VESA framebuffer display driver
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/mem.hpp>

#include <display.hpp>

using namespace cassio;
using namespace std;

Display::Display(u32* framebuffer, u32* backBuffer, u32 width, u32 height, u32 pitch)
    : framebuffer(framebuffer), backData(backBuffer), width(width), pitch(pitch), height(height),
      scrollOffset(0), dirty(false) {}

u32 Display::wrap(u32 y) const {
    return height > 0 ? (y + scrollOffset) % height : 0;
}

u32* Display::pixelAt(u32 x, u32 wrappedY) {
    return reinterpret_cast<u32*>(reinterpret_cast<u8*>(backData) + wrappedY * pitch) + x;
}

void Display::fillRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    dirty = true;
    for (u32 row = 0; row < h; ++row) {
        u32 wy = wrap(y + row);
        if (wy < height) {
            u32* dst = pixelAt(x, wy);
            for (u32 col = 0; col < w && x + col < width; ++col) {
                dst[col] = color;
            }
        }
    }
}

void Display::drawRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    if (w == 0 || h == 0) {
        return;
    }
    dirty = true;
    fillRect(x, y, w, 1, color);
    fillRect(x, y + h - 1, w, 1, color);
    if (h > 2) {
        fillRect(x, y + 1, 1, h - 2, color);
        fillRect(x + w - 1, y + 1, 1, h - 2, color);
    }
}

void Display::blit(u32 x, u32 y, u32 w, u32 h, const u32* pixels) {
    dirty = true;
    for (u32 row = 0; row < h; ++row) {
        u32 wy = wrap(y + row);
        if (wy < height) {
            u32* dst = pixelAt(x, wy);
            const u32* src = pixels + row * w;
            u32 copyW = w;
            if (x + copyW > width) {
                copyW = width - x;
            }
            mem::copy(dst, src, copyW * sizeof(u32));
        }
    }
}

void Display::drawChar(u32 x, u32 y, char ch, gfx::Color fg, gfx::Color bg) {
    dirty = true;
    const u8* glyph = gfx::font[static_cast<u8>(ch)];

    for (u32 row = 0; row < gfx::FONT_HEIGHT; ++row) {
        u32 wy = wrap(y + row);
        if (wy >= height) {
            continue;
        }
        u32* dst = pixelAt(x, wy);
        u8 bits = glyph[row];
        for (u32 col = 0; col < gfx::FONT_WIDTH && x + col < width; ++col) {
            dst[col] = (bits & (0x80 >> col)) ? fg : bg;
        }
    }
}

void Display::scroll(u32 pixels, gfx::Color color) {
    if (height == 0) {
        return;
    }
    dirty = true;

    // Clear the rows that are about to become the new bottom.
    // These are at the old top of the ring (current scrollOffset).
    for (u32 row = 0; row < pixels && row < height; ++row) {
        u32 clearRow = (scrollOffset + row) % height;
        u32* dst = pixelAt(0, clearRow);
        for (u32 col = 0; col < width; ++col) {
            dst[col] = color;
        }
    }

    // Advance the offset. No data movement.
    scrollOffset = (scrollOffset + pixels) % height;
}

void Display::flush() {
    if (!dirty) {
        return;
    }
    dirty = false;

    u32 firstRows = height - scrollOffset;
    u8* src = reinterpret_cast<u8*>(backData);
    u8* dst = reinterpret_cast<u8*>(framebuffer);

    // Copy back[scrollOffset..height) to framebuffer[0..firstRows).
    mem::copy(dst, src + scrollOffset * pitch, firstRows * pitch);

    // Copy back[0..scrollOffset) to framebuffer[firstRows..height).
    if (scrollOffset > 0) {
        mem::copy(dst + firstRows * pitch, src, scrollOffset * pitch);
    }
}

u32 Display::getWidth() const {
    return width;
}
u32 Display::getHeight() const {
    return height;
}
u32 Display::getPitch() const {
    return pitch;
}
u32 Display::getBpp() const {
    return 32;
}
