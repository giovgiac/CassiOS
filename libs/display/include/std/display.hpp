/**
 * display.hpp -- Display service client
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Instance-based client for the display service. The constructor
 * resolves the service PID from the nameserver automatically,
 * blocking until the service is registered.
 *
 */

#ifndef STD_DISPLAY_HPP
#define STD_DISPLAY_HPP

#include <std/gfx.hpp>
#include <std/types.hpp>

namespace std {
namespace display {

struct DisplayInfo {
    u32 width;
    u32 height;
    u32 pitch;
    u32 bpp;
};

class Display {
public:
    Display();

    void fillRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color);
    void drawRect(u32 x, u32 y, u32 w, u32 h, gfx::Color color);
    void blit(u32 x, u32 y, u32 w, u32 h, const u32* pixels);
    void scroll(u32 pixels, gfx::Color color);
    void flush();
    DisplayInfo getInfo();

    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

private:
    u32 pid;
};

} // namespace display
} // namespace std

#endif // STD_DISPLAY_HPP
