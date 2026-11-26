/**
 * main.cpp -- Hello world user program
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/display.hpp>
#include <std/gfx.hpp>
#include <std/os.hpp>
#include <std/str.hpp>

using namespace std;

extern "C" void _start() {
    str::StringView greet = "Hello, World!";
    display::Display disp;
    auto info = disp.getInfo();
    auto halfHeight = (info.height / gfx::FONT_HEIGHT) / 2;
    auto halfWidth = (info.width / gfx::FONT_WIDTH) / 2;
    disp.fillRect(0, 0, info.width, info.height, 0x00FF0000);
    for (u32 c = 0; c < greet.length(); c++)
        disp.drawChar((c + halfWidth - greet.length() / 2) * gfx::FONT_WIDTH,
                      halfHeight * gfx::FONT_HEIGHT, greet[c], 0x00FFFFFF, 0x00FF0000);
    disp.flush();

    // Return execution to the kernel and exit after 5 seconds.
    os::sleep(5000);
    disp.fillRect(0, 0, info.width, info.height, 0x00000000);
    os::exit(0);
}
