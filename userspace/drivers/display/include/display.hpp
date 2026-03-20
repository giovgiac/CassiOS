/**
 * display.hpp -- VESA framebuffer display driver
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Owns the VESA framebuffer and a ring-buffered back buffer. All
 * drawing operates directly on the back buffer. Scroll just
 * increments an offset (O(1), no data movement). flush() copies
 * the ring buffer to the linear framebuffer.
 *
 */

#ifndef DISPLAY_HPP_
#define DISPLAY_HPP_

#include <std/gfx.hpp>
#include <std/types.hpp>

namespace cassio {

class Display {
public:
    Display(std::u32* framebuffer, std::u32* backBuffer, std::u32 width, std::u32 height,
            std::u32 pitch);

    void fillRect(std::u32 x, std::u32 y, std::u32 w, std::u32 h, std::gfx::Color color);
    void drawRect(std::u32 x, std::u32 y, std::u32 w, std::u32 h, std::gfx::Color color);
    void blit(std::u32 x, std::u32 y, std::u32 w, std::u32 h, const std::u32* pixels);
    void drawChar(std::u32 x, std::u32 y, char ch, std::gfx::Color fg, std::gfx::Color bg);
    void scroll(std::u32 pixels, std::gfx::Color color);
    void flush();

    std::u32 getWidth() const;
    std::u32 getHeight() const;
    std::u32 getPitch() const;
    std::u32 getBpp() const;

    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

private:
    std::u32* framebuffer;
    std::u32* backData;
    std::u32 width;
    std::u32 pitch;
    std::u32 height;
    std::u32 scrollOffset; ///< Ring buffer offset in pixels (rows).
    bool dirty;

    /// Translate a screen Y coordinate to the wrapped back buffer Y.
    std::u32 wrap(std::u32 y) const;

    /// Get a pointer to pixel (x, wrappedY) in the back buffer.
    std::u32* pixelAt(std::u32 x, std::u32 wrappedY);
};

} // namespace cassio

#endif // DISPLAY_HPP_
