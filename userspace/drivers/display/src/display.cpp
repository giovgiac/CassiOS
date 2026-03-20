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
    : framebuffer(framebuffer), backBuf(backBuffer, width, height, pitch), dirty(false) {}

void Display::fillRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    dirty = true;
    backBuf.fillRect(x, y, w, h, color);
}

void Display::drawRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    dirty = true;
    backBuf.drawRect(x, y, w, h, color);
}

void Display::blit(u32 x, u32 y, u32 w, u32 h, const u32* pixels) {
    dirty = true;
    gfx::PixelBuffer src(const_cast<u32*>(pixels), w, h, w * sizeof(u32));
    backBuf.blit(x, y, src, 0, 0, w, h);
}

void Display::drawChar(u32 x, u32 y, char ch, gfx::Color fg, gfx::Color bg) {
    dirty = true;
    backBuf.drawChar(x, y, ch, fg, bg);
}

void Display::scroll(u32 pixels, gfx::Color color) {
    dirty = true;
    backBuf.scroll(pixels, color);
}

void Display::flush() {
    if (!dirty) {
        return;
    }
    dirty = false;

    u32 offset = backBuf.getScrollOffset();
    u32 height = backBuf.getHeight();
    u32 pitch = backBuf.getPitch();
    u8* src = reinterpret_cast<u8*>(backBuf.getData());
    u8* dst = reinterpret_cast<u8*>(framebuffer);

    u32 firstRows = height - offset;

    // Copy back[offset..height) to framebuffer[0..firstRows).
    mem::copy(dst, src + offset * pitch, firstRows * pitch);

    // Copy back[0..offset) to framebuffer[firstRows..height).
    if (offset > 0) {
        mem::copy(dst + firstRows * pitch, src, offset * pitch);
    }
}

u32 Display::getWidth() const {
    return backBuf.getWidth();
}
u32 Display::getHeight() const {
    return backBuf.getHeight();
}
u32 Display::getPitch() const {
    return backBuf.getPitch();
}
u32 Display::getBpp() const {
    return 32;
}
