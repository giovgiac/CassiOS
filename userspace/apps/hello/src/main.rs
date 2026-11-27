// main.rs -- hello world user program
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Fills the screen red with "Hello, World!" in the middle, waits five
// seconds, clears the screen and exits.

#![cfg_attr(not(test), no_std)]

mod greeting;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::display::Display;
    use cassio::os;

    const RED: u32 = 0x00FF_0000;
    const WHITE: u32 = 0x00FF_FFFF;
    const BLACK: u32 = 0x0000_0000;

    let display = Display::connect();
    let (width, height) = display.size();
    display.fill_rect(0, 0, width, height, RED);
    for (i, &ch) in greeting::TEXT.iter().enumerate() {
        let (x, y) = greeting::position(width, height, i);
        display.draw_char(x, y, ch, WHITE, RED);
    }
    display.flush();

    os::sleep(5000);
    display.fill_rect(0, 0, width, height, BLACK);
    display.flush();
    os::exit(0)
}
