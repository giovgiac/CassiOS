/**
 * display.cpp -- VESA framebuffer display driver
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <display.hpp>

#include <std/mem.hpp>

using namespace cassio;
using namespace std;

Display::Display(u32* framebuffer, u32* backBuffer, u32 width, u32 height, u32 pitch)
    : framebuffer(framebuffer), backBuf(backBuffer, width, height, pitch), pitch(pitch),
      height(height), scrollOffset(0) {}

u32 Display::wrap(u32 y) const { return height > 0 ? (y + scrollOffset) % height : 0; }

void Display::fillRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    // Row-by-row to handle wrapping.
    for (u32 row = 0; row < h; ++row) {
        u32 wy = wrap(y + row);
        if (wy < height) {
            u32* dst = reinterpret_cast<u32*>(reinterpret_cast<u8*>(backBuf.getData()) +
                                              wy * pitch) +
                       x;
            for (u32 col = 0; col < w && x + col < backBuf.getWidth(); ++col) {
                dst[col] = color;
            }
        }
    }
}

void Display::drawRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    if (w == 0 || h == 0) {
        return;
    }
    fillRect(x, y, w, 1, color);
    fillRect(x, y + h - 1, w, 1, color);
    if (h > 2) {
        fillRect(x, y + 1, 1, h - 2, color);
        fillRect(x + w - 1, y + 1, 1, h - 2, color);
    }
}

void Display::blit(u32 x, u32 y, u32 w, u32 h, const u32* pixels) {
    for (u32 row = 0; row < h; ++row) {
        u32 wy = wrap(y + row);
        if (wy < height) {
            u32* dst = reinterpret_cast<u32*>(reinterpret_cast<u8*>(backBuf.getData()) +
                                              wy * pitch) +
                       x;
            const u32* src = pixels + row * w;
            u32 copyW = w;
            if (x + copyW > backBuf.getWidth()) {
                copyW = backBuf.getWidth() - x;
            }
            mem::copy(dst, src, copyW * sizeof(u32));
        }
    }
}

void Display::drawChar(u32 x, u32 y, char ch, gfx::Color fg, gfx::Color bg) {
    const u8* glyph = gfx::font[static_cast<u8>(ch)];
    u32 bufWidth = backBuf.getWidth();

    for (u32 row = 0; row < gfx::FONT_HEIGHT; ++row) {
        u32 wy = wrap(y + row);
        if (wy >= height) {
            continue;
        }
        u32* dst = reinterpret_cast<u32*>(reinterpret_cast<u8*>(backBuf.getData()) + wy * pitch) +
                   x;
        u8 bits = glyph[row];
        for (u32 col = 0; col < gfx::FONT_WIDTH && x + col < bufWidth; ++col) {
            dst[col] = (bits & (0x80 >> col)) ? fg : bg;
        }
    }
}

void Display::scroll(u32 pixels, gfx::Color color) {
    if (height == 0) {
        return;
    }

    // Clear the rows that are about to become the new bottom.
    // These are at the old top of the ring (current scrollOffset).
    for (u32 row = 0; row < pixels && row < height; ++row) {
        u32 clearRow = (scrollOffset + row) % height;
        u32* dst =
            reinterpret_cast<u32*>(reinterpret_cast<u8*>(backBuf.getData()) + clearRow * pitch);
        u32 w = backBuf.getWidth();
        for (u32 col = 0; col < w; ++col) {
            dst[col] = color;
        }
    }

    // Advance the offset. No data movement.
    scrollOffset = (scrollOffset + pixels) % height;
}

void Display::flush() {
    u32 firstRows = height - scrollOffset;
    u8* src = reinterpret_cast<u8*>(backBuf.getData());
    u8* dst = reinterpret_cast<u8*>(framebuffer);

    // Copy back[scrollOffset..height) to framebuffer[0..firstRows).
    mem::copy(dst, src + scrollOffset * pitch, firstRows * pitch);

    // Copy back[0..scrollOffset) to framebuffer[firstRows..height).
    if (scrollOffset > 0) {
        mem::copy(dst + firstRows * pitch, src, scrollOffset * pitch);
    }
}

u32 Display::getWidth() const { return backBuf.getWidth(); }
u32 Display::getHeight() const { return height; }
u32 Display::getPitch() const { return pitch; }
u32 Display::getBpp() const { return 32; }
