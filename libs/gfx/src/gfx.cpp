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

static inline u32* pixelAt(gfx::PixelBuffer& buf, u32 x, u32 y) {
    return reinterpret_cast<u32*>(reinterpret_cast<u8*>(buf.data) + y * buf.pitch) + x;
}

static inline const u32* pixelAt(const gfx::PixelBuffer& buf, u32 x, u32 y) {
    return reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(buf.data) + y * buf.pitch) + x;
}

void gfx::drawPixel(PixelBuffer& buf, u32 x, u32 y, Color color) {
    if (x < buf.width && y < buf.height) {
        *pixelAt(buf, x, y) = color;
    }
}

void gfx::fillRect(PixelBuffer& buf, u32 x, u32 y, u32 w, u32 h, Color color) {
    // Clip to buffer bounds.
    if (x >= buf.width || y >= buf.height) {
        return;
    }
    if (x + w > buf.width) {
        w = buf.width - x;
    }
    if (y + h > buf.height) {
        h = buf.height - y;
    }

    for (u32 row = 0; row < h; ++row) {
        u32* dst = pixelAt(buf, x, y + row);
        for (u32 col = 0; col < w; ++col) {
            dst[col] = color;
        }
    }
}

void gfx::drawRect(PixelBuffer& buf, u32 x, u32 y, u32 w, u32 h, Color color) {
    if (w == 0 || h == 0) {
        return;
    }
    // Top and bottom edges.
    fillRect(buf, x, y, w, 1, color);
    fillRect(buf, x, y + h - 1, w, 1, color);
    // Left and right edges (excluding corners already drawn).
    if (h > 2) {
        fillRect(buf, x, y + 1, 1, h - 2, color);
        fillRect(buf, x + w - 1, y + 1, 1, h - 2, color);
    }
}

void gfx::drawChar(PixelBuffer& buf, u32 x, u32 y, char ch, Color fg, Color bg) {
    const u8* glyph = font[static_cast<u8>(ch)];

    for (u32 row = 0; row < FONT_HEIGHT; ++row) {
        u32 py = y + row;
        if (py >= buf.height) {
            break;
        }
        u8 bits = glyph[row];
        for (u32 col = 0; col < FONT_WIDTH; ++col) {
            u32 px = x + col;
            if (px >= buf.width) {
                break;
            }
            // MSB = leftmost pixel.
            Color c = (bits & (0x80 >> col)) ? fg : bg;
            *pixelAt(buf, px, py) = c;
        }
    }
}

void gfx::drawText(PixelBuffer& buf, u32 x, u32 y, const char* text, u32 len, Color fg, Color bg) {
    for (u32 i = 0; i < len; ++i) {
        drawChar(buf, x + i * FONT_WIDTH, y, text[i], fg, bg);
    }
}

void gfx::scroll(PixelBuffer& buf, u32 pixels, Color color) {
    if (pixels == 0) {
        return;
    }
    if (pixels >= buf.height) {
        fillRect(buf, 0, 0, buf.width, buf.height, color);
        return;
    }

    // Shift rows up.
    for (u32 row = 0; row < buf.height - pixels; ++row) {
        u8* dst = reinterpret_cast<u8*>(buf.data) + row * buf.pitch;
        const u8* src = reinterpret_cast<const u8*>(buf.data) + (row + pixels) * buf.pitch;
        mem::copy(dst, src, buf.width * sizeof(u32));
    }

    // Fill the gap at the bottom.
    fillRect(buf, 0, buf.height - pixels, buf.width, pixels, color);
}

void gfx::blit(PixelBuffer& dst, u32 dx, u32 dy, const PixelBuffer& src, u32 sx, u32 sy, u32 w,
               u32 h) {
    // Clip source region.
    if (sx >= src.width || sy >= src.height) {
        return;
    }
    if (sx + w > src.width) {
        w = src.width - sx;
    }
    if (sy + h > src.height) {
        h = src.height - sy;
    }

    // Clip destination region.
    if (dx >= dst.width || dy >= dst.height) {
        return;
    }
    if (dx + w > dst.width) {
        w = dst.width - dx;
    }
    if (dy + h > dst.height) {
        h = dst.height - dy;
    }

    for (u32 row = 0; row < h; ++row) {
        const u32* srcRow = pixelAt(src, sx, sy + row);
        u32* dstRow = pixelAt(dst, dx, dy + row);
        mem::copy(dstRow, srcRow, w * sizeof(u32));
    }
}
