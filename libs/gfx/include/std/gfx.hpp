/**
 * gfx.hpp -- Stateless 2D graphics primitives
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * PixelBuffer wraps a caller-provided pixel buffer with drawing
 * methods. Non-owning -- does not allocate or free the memory.
 * Supports O(1) scrolling via an internal ring buffer offset.
 * All drawing methods clip to buffer bounds.
 *
 */

#ifndef STD_GFX_HPP
#define STD_GFX_HPP

#include <std/str.hpp>
#include <std/types.hpp>

namespace std {
namespace gfx {

using Color = u32;

static constexpr u32 FONT_WIDTH = 8;
static constexpr u32 FONT_HEIGHT = 16;

class PixelBuffer {
public:
    PixelBuffer(u32* data, u32 width, u32 height, u32 pitch);

    void drawPixel(u32 x, u32 y, Color color);
    void fillRect(u32 x, u32 y, u32 w, u32 h, Color color);
    void drawRect(u32 x, u32 y, u32 w, u32 h, Color color);
    void drawChar(u32 x, u32 y, char ch, Color fg, Color bg);
    void drawText(u32 x, u32 y, str::StringView text, Color fg, Color bg);
    void scroll(u32 pixels, Color color);
    void blit(u32 dx, u32 dy, const PixelBuffer& src, u32 sx, u32 sy, u32 w, u32 h);

    u32 getWidth() const;
    u32 getHeight() const;
    u32 getPitch() const;
    u32* getData() const;
    u32 getScrollOffset() const;

private:
    u32* data;
    u32 width;
    u32 height;
    u32 pitch;        // Bytes per scanline.
    u32 scrollOffset; // Ring buffer offset in rows.

    // Translate a logical Y coordinate to the wrapped buffer Y.
    u32 wrap(u32 y) const;

    u32* pixelAt(u32 x, u32 y);
    const u32* pixelAt(u32 x, u32 y) const;
};

extern const u8 font[256][16];

} // namespace gfx
} // namespace std

#endif // STD_GFX_HPP
