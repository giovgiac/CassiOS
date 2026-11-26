// main.rs -- display service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Userspace framebuffer driver. Maps the VESA framebuffer from the boot
// loader, draws into a back buffer on request, and copies it to the
// screen on DisplayFlush.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

mod display;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use core::mem::size_of_val;
    use core::slice;

    use cassio::ipc::{self, Message, message_type};
    use cassio::{ns, os};

    use display::Display;

    // Userspace address the framebuffer is mapped at.
    const FB_VIRT: u32 = 0x1000_0000;
    const PAGE_SIZE: u32 = 4096;
    // Largest DisplayBlit, in pixels: two 8x16 glyph cells.
    const BLIT_MAX: usize = 256;

    let fb = os::framebuffer_info();
    let fb_bytes = fb.pitch * fb.height;
    os::map_device(fb.address, FB_VIRT, fb_bytes.div_ceil(PAGE_SIZE));
    let framebuffer =
        unsafe { slice::from_raw_parts_mut(FB_VIRT as *mut u32, (fb_bytes / 4) as usize) };

    // Without memory for the back buffer, run as a 0x0 display so clients
    // still get replies.
    let mut display = Display::new(framebuffer, fb.width, fb.height, fb.pitch)
        .or_else(|| {
            os::write(2, b"display: no memory for the back buffer\n");
            Display::new(&mut [], 0, 0, 0)
        })
        .unwrap();
    display.fill_rect(0, 0, fb.width, fb.height, 0x000000);
    display.flush();
    ns::register_name(b"display");

    loop {
        // Zeroed per request: receive does not report how many bytes
        // arrived, so a short blit must not reuse stale pixels.
        let mut pixels = [0u32; BLIT_MAX];
        let bytes = unsafe {
            slice::from_raw_parts_mut(pixels.as_mut_ptr().cast::<u8>(), size_of_val(&pixels))
        };
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, bytes);
        let [x, y, a3, a4, a5] = [msg.arg1, msg.arg2, msg.arg3, msg.arg4, msg.arg5];

        // Drawing requests arrive by notify and get no reply; blit,
        // get-info and unknown requests are sends and are answered with
        // the display's dimensions.
        match msg.kind {
            message_type::DISPLAY_FILL_RECT => display.fill_rect(x, y, a3, a4, a5),
            message_type::DISPLAY_DRAW_RECT => display.draw_rect(x, y, a3, a4, a5),
            message_type::DISPLAY_DRAW_CHAR => display.draw_char(x, y, a3 as u8, a4, a5),
            message_type::DISPLAY_SCROLL => display.scroll(x, y),
            message_type::DISPLAY_FLUSH => display.flush(),
            _ if sender > 0 => {
                if msg.kind == message_type::DISPLAY_BLIT {
                    display.blit(x, y, a3, a4, &pixels);
                }
                let (width, height, pitch, bpp) = display.info();
                let mut reply = Message {
                    arg1: width,
                    arg2: height,
                    arg3: pitch,
                    arg4: bpp,
                    ..Message::default()
                };
                ipc::reply(sender as u32, &mut reply, &[]);
            }
            _ => {}
        }
    }
}
