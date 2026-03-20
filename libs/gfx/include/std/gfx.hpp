/**
 * gfx.hpp -- Stateless 2D graphics primitives
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Pure pixel manipulation on caller-provided buffers. No ownership,
 * no display knowledge, no IPC. All functions clip to buffer bounds.
 *
 */

#ifndef STD_GFX_HPP
#define STD_GFX_HPP

#include <std/types.hpp>

namespace std {
namespace gfx {

using Color = u32;

static constexpr u32 FONT_WIDTH = 8;
static constexpr u32 FONT_HEIGHT = 16;

struct PixelBuffer {
    u32* data;
    u32 width;
    u32 height;
    u32 pitch; ///< Bytes per scanline.
};

void drawPixel(PixelBuffer& buf, u32 x, u32 y, Color color);
void fillRect(PixelBuffer& buf, u32 x, u32 y, u32 w, u32 h, Color color);
void drawRect(PixelBuffer& buf, u32 x, u32 y, u32 w, u32 h, Color color);
void drawChar(PixelBuffer& buf, u32 x, u32 y, char ch, Color fg, Color bg);
void drawText(PixelBuffer& buf, u32 x, u32 y, const char* text, u32 len, Color fg, Color bg);
void scroll(PixelBuffer& buf, u32 pixels, Color color);
void blit(PixelBuffer& dst, u32 dx, u32 dy, const PixelBuffer& src, u32 sx, u32 sy, u32 w, u32 h);

extern const u8 font[256][16];

} // namespace gfx
} // namespace std

#endif // STD_GFX_HPP
