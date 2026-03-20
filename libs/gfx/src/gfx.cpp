/**
 * gfx.cpp -- Stateless 2D graphics primitives
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/gfx.hpp>

#include <std/mem.hpp>

using namespace std;
using namespace std::gfx;

PixelBuffer::PixelBuffer(u32* data, u32 width, u32 height, u32 pitch)
    : data(data), width(width), height(height), pitch(pitch) {}

u32* PixelBuffer::pixelAt(u32 x, u32 y) {
    return reinterpret_cast<u32*>(reinterpret_cast<u8*>(data) + y * pitch) + x;
}

const u32* PixelBuffer::pixelAt(u32 x, u32 y) const {
    return reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(data) + y * pitch) + x;
}

u32 PixelBuffer::getWidth() const { return width; }
u32 PixelBuffer::getHeight() const { return height; }
u32 PixelBuffer::getPitch() const { return pitch; }
u32* PixelBuffer::getData() const { return data; }

void PixelBuffer::drawPixel(u32 x, u32 y, Color color) {
    if (x < width && y < height) {
        *pixelAt(x, y) = color;
    }
}

void PixelBuffer::fillRect(u32 x, u32 y, u32 w, u32 h, Color color) {
    if (x >= width || y >= height) {
        return;
    }
    if (x + w > width) {
        w = width - x;
    }
    if (y + h > height) {
        h = height - y;
    }

    for (u32 row = 0; row < h; ++row) {
        u32* dst = pixelAt(x, y + row);
        for (u32 col = 0; col < w; ++col) {
            dst[col] = color;
        }
    }
}

void PixelBuffer::drawRect(u32 x, u32 y, u32 w, u32 h, Color color) {
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

void PixelBuffer::drawChar(u32 x, u32 y, char ch, Color fg, Color bg) {
    const u8* glyph = font[static_cast<u8>(ch)];

    for (u32 row = 0; row < FONT_HEIGHT; ++row) {
        u32 py = y + row;
        if (py >= height) {
            break;
        }
        u8 bits = glyph[row];
        for (u32 col = 0; col < FONT_WIDTH; ++col) {
            u32 px = x + col;
            if (px >= width) {
                break;
            }
            Color c = (bits & (0x80 >> col)) ? fg : bg;
            *pixelAt(px, py) = c;
        }
    }
}

void PixelBuffer::drawText(u32 x, u32 y, const char* text, u32 len, Color fg, Color bg) {
    for (u32 i = 0; i < len; ++i) {
        drawChar(x + i * FONT_WIDTH, y, text[i], fg, bg);
    }
}

void PixelBuffer::scroll(u32 pixels, Color color) {
    if (pixels == 0) {
        return;
    }
    if (pixels >= height) {
        fillRect(0, 0, width, height, color);
        return;
    }

    for (u32 row = 0; row < height - pixels; ++row) {
        u8* dst = reinterpret_cast<u8*>(data) + row * pitch;
        const u8* src = reinterpret_cast<const u8*>(data) + (row + pixels) * pitch;
        mem::copy(dst, src, width * sizeof(u32));
    }

    fillRect(0, height - pixels, width, pixels, color);
}

void PixelBuffer::blit(u32 dx, u32 dy, const PixelBuffer& src, u32 sx, u32 sy, u32 w, u32 h) {
    if (sx >= src.width || sy >= src.height) {
        return;
    }
    if (sx + w > src.width) {
        w = src.width - sx;
    }
    if (sy + h > src.height) {
        h = src.height - sy;
    }

    if (dx >= width || dy >= height) {
        return;
    }
    if (dx + w > width) {
        w = width - dx;
    }
    if (dy + h > height) {
        h = height - dy;
    }

    for (u32 row = 0; row < h; ++row) {
        const u32* srcRow = src.pixelAt(sx, sy + row);
        u32* dstRow = pixelAt(dx, dy + row);
        mem::copy(dstRow, srcRow, w * sizeof(u32));
    }
}
