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
    : framebuffer(framebuffer), backBuf(backBuffer, width, height, pitch), fbSize(pitch * height),
      dirtyTop(height), dirtyBottom(0) {}

void Display::markDirty(u32 y, u32 h) {
    if (y < dirtyTop) {
        dirtyTop = y;
    }
    u32 bottom = y + h;
    if (bottom > backBuf.getHeight()) {
        bottom = backBuf.getHeight();
    }
    if (bottom > dirtyBottom) {
        dirtyBottom = bottom;
    }
}

void Display::fillRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    backBuf.fillRect(x, y, w, h, color);
    markDirty(y, h);
}

void Display::drawRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    backBuf.drawRect(x, y, w, h, color);
    markDirty(y, h);
}

void Display::blit(u32 x, u32 y, u32 w, u32 h, const u32* pixels) {
    gfx::PixelBuffer src(const_cast<u32*>(pixels), w, h, w * sizeof(u32));
    backBuf.blit(x, y, src, 0, 0, w, h);
    markDirty(y, h);
}

void Display::drawChar(u32 x, u32 y, char ch, gfx::Color fg, gfx::Color bg) {
    backBuf.drawChar(x, y, ch, fg, bg);
    markDirty(y, gfx::FONT_HEIGHT);
}

void Display::scroll(u32 pixels, gfx::Color color) {
    backBuf.scroll(pixels, color);

    // Apply the same scroll to the framebuffer so we don't have to
    // flush the entire buffer. Only the new bottom rows are dirty.
    u32 height = backBuf.getHeight();
    u32 pitch = backBuf.getPitch();
    if (pixels < height) {
        u32 copyRows = height - pixels;
        mem::copy(framebuffer, reinterpret_cast<u8*>(framebuffer) + pixels * pitch,
                  copyRows * pitch);
    }
    markDirty(height - pixels, pixels);
}

void Display::flush() {
    if (dirtyTop >= dirtyBottom) {
        return; // Nothing dirty.
    }

    u32 pitch = backBuf.getPitch();
    u8* dst = reinterpret_cast<u8*>(framebuffer) + dirtyTop * pitch;
    const u8* src = reinterpret_cast<const u8*>(backBuf.getData()) + dirtyTop * pitch;
    u32 bytes = (dirtyBottom - dirtyTop) * pitch;
    mem::copy(dst, src, bytes);

    // Reset dirty region.
    dirtyTop = backBuf.getHeight();
    dirtyBottom = 0;
}

u32 Display::getWidth() const { return backBuf.getWidth(); }
u32 Display::getHeight() const { return backBuf.getHeight(); }
u32 Display::getPitch() const { return backBuf.getPitch(); }
u32 Display::getBpp() const { return 32; }
