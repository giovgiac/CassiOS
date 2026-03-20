/**
 * display.hpp -- VESA framebuffer display driver
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Owns the VESA framebuffer and a back buffer. All drawing
 * operates on the back buffer via std::gfx::PixelBuffer.
 * flush() copies the back buffer to the framebuffer.
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
    std::gfx::PixelBuffer backBuf;
    std::u32 fbSize; ///< Total framebuffer size in bytes (pitch * height).
};

} // namespace cassio

#endif // DISPLAY_HPP_
