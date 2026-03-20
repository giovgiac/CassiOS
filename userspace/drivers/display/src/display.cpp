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
    : framebuffer(framebuffer), backBuf(backBuffer, width, height, pitch),
      fbSize(pitch * height) {}

void Display::fillRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    backBuf.fillRect(x, y, w, h, color);
}

void Display::drawRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color) {
    backBuf.drawRect(x, y, w, h, color);
}

void Display::blit(u32 x, u32 y, u32 w, u32 h, const u32* pixels) {
    gfx::PixelBuffer src(const_cast<u32*>(pixels), w, h, w * sizeof(u32));
    backBuf.blit(x, y, src, 0, 0, w, h);
}

void Display::scroll(u32 pixels, gfx::Color color) {
    backBuf.scroll(pixels, color);
}

void Display::flush() {
    mem::copy(framebuffer, backBuf.getData(), fbSize);
}

u32 Display::getWidth() const { return backBuf.getWidth(); }
u32 Display::getHeight() const { return backBuf.getHeight(); }
u32 Display::getPitch() const { return backBuf.getPitch(); }
u32 Display::getBpp() const { return 32; }
