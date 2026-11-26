// display.rs -- double-buffered framebuffer
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::gfx::{Color, PixelBuffer};

/// Bits per pixel the display draws with.
pub const BPP: u32 = 32;

/// Draws into a back buffer and copies it to the framebuffer on `flush`,
/// so clients control when the screen updates.
pub struct Display<'a> {
    framebuffer: &'a mut [u32],
    /// Bytes per framebuffer scanline.
    pitch: u32,
    back: PixelBuffer,
    dirty: bool,
}

impl<'a> Display<'a> {
    /// A display over `framebuffer` (`height` rows `pitch` bytes apart),
    /// or None if the back buffer cannot be allocated.
    pub fn new(framebuffer: &'a mut [u32], width: u32, height: u32, pitch: u32) -> Option<Self> {
        Some(Self {
            framebuffer,
            pitch,
            back: PixelBuffer::new(width, height)?,
            dirty: false,
        })
    }

    pub fn fill_rect(&mut self, x: u32, y: u32, w: u32, h: u32, color: Color) {
        self.dirty = true;
        self.back.fill_rect(x, y, w, h, color);
    }

    pub fn draw_rect(&mut self, x: u32, y: u32, w: u32, h: u32, color: Color) {
        self.dirty = true;
        self.back.draw_rect(x, y, w, h, color);
    }

    pub fn draw_char(&mut self, x: u32, y: u32, ch: u8, fg: Color, bg: Color) {
        self.dirty = true;
        self.back.draw_char(x, y, ch, fg, bg);
    }

    pub fn scroll(&mut self, rows: u32, color: Color) {
        self.dirty = true;
        self.back.scroll(rows, color);
    }

    pub fn blit(&mut self, x: u32, y: u32, w: u32, h: u32, pixels: &[Color]) {
        self.dirty = true;
        self.back.blit(x, y, w, h, pixels);
    }

    /// Copy the back buffer to the framebuffer if anything was drawn.
    pub fn flush(&mut self) {
        if self.dirty {
            self.dirty = false;
            self.back
                .copy_to(self.framebuffer, (self.pitch / 4) as usize);
        }
    }

    /// Width, height, pitch (bytes) and bits per pixel.
    pub fn info(&self) -> (u32, u32, u32, u32) {
        (self.back.width(), self.back.height(), self.pitch, BPP)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const W: u32 = 16;
    const H: u32 = 8;
    const PITCH: u32 = W * 4;

    fn at(fb: &[u32], x: u32, y: u32) -> u32 {
        fb[(y * W + x) as usize]
    }

    #[test]
    fn fill_rect_then_flush() {
        let mut fb = [0u32; (W * H) as usize];
        let mut display = Display::new(&mut fb, W, H, PITCH).unwrap();
        display.fill_rect(1, 1, 3, 2, 0xAA);
        display.flush();
        assert_eq!(
            (at(&fb, 1, 1), at(&fb, 3, 2), at(&fb, 0, 0)),
            (0xAA, 0xAA, 0)
        );
    }

    #[test]
    fn nothing_reaches_the_framebuffer_before_flush() {
        let mut fb = [0u32; (W * H) as usize];
        let mut display = Display::new(&mut fb, W, H, PITCH).unwrap();
        display.fill_rect(0, 0, W, H, 0xAA);
        drop(display);
        assert!(fb.iter().all(|&p| p == 0));
    }

    #[test]
    fn draw_rect() {
        let mut fb = [0u32; (W * H) as usize];
        let mut display = Display::new(&mut fb, W, H, PITCH).unwrap();
        display.draw_rect(0, 0, 4, 3, 0xCC);
        display.flush();
        assert_eq!(
            (at(&fb, 0, 0), at(&fb, 3, 0), at(&fb, 0, 2), at(&fb, 1, 1)),
            (0xCC, 0xCC, 0xCC, 0)
        );
    }

    #[test]
    fn scroll_shifts_content() {
        let mut fb = [0u32; (W * H) as usize];
        let mut display = Display::new(&mut fb, W, H, PITCH).unwrap();
        display.fill_rect(0, 2, W, 1, 0xDD);
        display.scroll(1, 0x00);
        display.flush();
        assert_eq!((at(&fb, 0, 1), at(&fb, 0, H - 1)), (0xDD, 0));
    }

    #[test]
    fn scroll_fills_bottom() {
        let mut fb = [0u32; (W * H) as usize];
        let mut display = Display::new(&mut fb, W, H, PITCH).unwrap();
        display.fill_rect(0, 0, W, H, 0xFF);
        display.scroll(2, 0xAA);
        display.flush();
        assert_eq!(
            (at(&fb, 0, H - 2), at(&fb, 0, H - 1), at(&fb, 0, 0)),
            (0xAA, 0xAA, 0xFF)
        );
    }

    #[test]
    fn draw_after_scroll() {
        let mut fb = [0u32; (W * H) as usize];
        let mut display = Display::new(&mut fb, W, H, PITCH).unwrap();
        display.scroll(2, 0x00);
        display.fill_rect(0, H - 1, W, 1, 0xEE);
        display.flush();
        assert_eq!((at(&fb, 0, H - 1), at(&fb, 0, 0)), (0xEE, 0));
    }

    #[test]
    fn draw_char() {
        let mut fb = [0u32; (W * H * 2) as usize];
        let mut display = Display::new(&mut fb, W, H * 2, PITCH).unwrap();
        display.draw_char(0, 0, b'-', 0x77, 0x11);
        display.flush();
        // '-' is a single row of set pixels in an otherwise empty cell.
        let rows: Vec<u32> = (0..16).map(|y| at(&fb, 1, y)).collect();
        assert_eq!(rows.iter().filter(|&&p| p == 0x77).count(), 1);
        assert_eq!(rows.iter().filter(|&&p| p == 0x11).count(), 15);
        assert_eq!(at(&fb, 8, 0), 0); // outside the cell
    }

    #[test]
    fn blit() {
        let mut fb = [0u32; (W * H) as usize];
        let mut display = Display::new(&mut fb, W, H, PITCH).unwrap();
        display.blit(1, 1, 2, 2, &[0x11, 0x22, 0x33, 0x44]);
        display.flush();
        assert_eq!(
            (at(&fb, 1, 1), at(&fb, 2, 1), at(&fb, 1, 2), at(&fb, 2, 2)),
            (0x11, 0x22, 0x33, 0x44)
        );
    }

    #[test]
    fn framebuffer_pitch_wider_than_width() {
        // 4x2 display on a framebuffer with 6 pixels per scanline.
        let mut fb = [0u32; 12];
        let mut display = Display::new(&mut fb, 4, 2, 6 * 4).unwrap();
        display.fill_rect(0, 0, 4, 2, 0x55);
        display.flush();
        assert_eq!(
            fb,
            [0x55, 0x55, 0x55, 0x55, 0, 0, 0x55, 0x55, 0x55, 0x55, 0, 0]
        );
    }

    #[test]
    fn info() {
        let mut fb = [0u32; (W * H) as usize];
        let display = Display::new(&mut fb, W, H, PITCH).unwrap();
        assert_eq!(display.info(), (W, H, PITCH, 32));
    }
}
