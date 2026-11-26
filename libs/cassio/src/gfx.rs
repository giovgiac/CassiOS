// gfx.rs -- 32-bit pixel buffer with text and O(1) scrolling
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Rows form a ring: scrolling moves the logical top instead of copying
// pixels, and `copy_to` writes the rows out in logical order.

use alloc::vec::Vec;

mod font;

pub use font::FONT;

/// A 0x00RRGGBB color.
pub type Color = u32;

pub const FONT_WIDTH: u32 = 8;
pub const FONT_HEIGHT: u32 = 16;

/// A `width` x `height` grid of pixels, stored row by row.
pub struct PixelBuffer {
    pixels: Vec<u32>,
    width: u32,
    height: u32,
    /// Physical row holding logical row 0.
    top: u32,
}

impl PixelBuffer {
    /// A buffer cleared to 0, or None if it cannot be allocated.
    pub fn new(width: u32, height: u32) -> Option<Self> {
        let len = width as usize * height as usize;
        let mut pixels = Vec::new();
        pixels.try_reserve_exact(len).ok()?;
        pixels.resize(len, 0);
        Some(Self {
            pixels,
            width,
            height,
            top: 0,
        })
    }

    pub fn width(&self) -> u32 {
        self.width
    }

    pub fn height(&self) -> u32 {
        self.height
    }

    /// The pixel at logical position (`x`, `y`), which must be in bounds.
    pub fn pixel(&self, x: u32, y: u32) -> Color {
        self.pixels[self.index(x, y)]
    }

    /// Fill a rectangle, clipped to the buffer.
    pub fn fill_rect(&mut self, x: u32, y: u32, w: u32, h: u32, color: Color) {
        if x >= self.width || y >= self.height {
            return;
        }
        let w = w.min(self.width - x);
        let h = h.min(self.height - y);
        for row in y..y + h {
            let start = self.index(x, row);
            self.pixels[start..start + w as usize].fill(color);
        }
    }

    /// Draw a one-pixel rectangle outline, clipped to the buffer.
    pub fn draw_rect(&mut self, x: u32, y: u32, w: u32, h: u32, color: Color) {
        if w == 0 || h == 0 {
            return;
        }
        self.fill_rect(x, y, w, 1, color);
        self.fill_rect(x, y.saturating_add(h - 1), w, 1, color);
        if h > 2 {
            let y = y.saturating_add(1);
            self.fill_rect(x, y, 1, h - 2, color);
            self.fill_rect(x.saturating_add(w - 1), y, 1, h - 2, color);
        }
    }

    /// Draw glyph `ch` with its top-left corner at (`x`, `y`), clipped.
    pub fn draw_char(&mut self, x: u32, y: u32, ch: u8, fg: Color, bg: Color) {
        for (row, bits) in FONT[ch as usize].iter().enumerate() {
            let py = y.saturating_add(row as u32);
            if py >= self.height {
                break;
            }
            for col in 0..FONT_WIDTH {
                let px = x.saturating_add(col);
                if px >= self.width {
                    break;
                }
                let color = if bits & (0x80 >> col) != 0 { fg } else { bg };
                let i = self.index(px, py);
                self.pixels[i] = color;
            }
        }
    }

    /// Scroll the content up by `rows`, filling the new bottom rows with
    /// `color`. Scrolling the full height or more clears the buffer.
    pub fn scroll(&mut self, rows: u32, color: Color) {
        if rows == 0 || self.height == 0 {
            return;
        }
        if rows >= self.height {
            self.top = 0;
            self.pixels.fill(color);
            return;
        }
        // The old top rows become the new bottom rows.
        for row in 0..rows {
            let start = self.index(0, row);
            self.pixels[start..start + self.width as usize].fill(color);
        }
        self.top = (self.top + rows) % self.height;
    }

    /// Copy a `w` x `h` image (row-major, `w` pixels per row) to
    /// (`x`, `y`), clipped to the buffer and to the rows `pixels` holds.
    pub fn blit(&mut self, x: u32, y: u32, w: u32, h: u32, pixels: &[Color]) {
        if w == 0 || x >= self.width || y >= self.height {
            return;
        }
        let h = h
            .min((pixels.len() / w as usize) as u32)
            .min(self.height - y);
        let visible = w.min(self.width - x) as usize;
        for (row, src) in pixels.chunks_exact(w as usize).take(h as usize).enumerate() {
            let start = self.index(x, y + row as u32);
            self.pixels[start..start + visible].copy_from_slice(&src[..visible]);
        }
    }

    /// Write the buffer in logical row order into `dst`, whose rows start
    /// `dst_stride` pixels apart.
    pub fn copy_to(&self, dst: &mut [u32], dst_stride: usize) {
        let width = self.width as usize;
        for row in 0..self.height {
            let src = self.index(0, row);
            let start = row as usize * dst_stride;
            dst[start..start + width].copy_from_slice(&self.pixels[src..src + width]);
        }
    }

    /// Index of logical (`x`, `y`) in `pixels`.
    fn index(&self, x: u32, y: u32) -> usize {
        let row = (y + self.top) % self.height;
        row as usize * self.width as usize + x as usize
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn buffer(width: u32, height: u32) -> PixelBuffer {
        PixelBuffer::new(width, height).unwrap()
    }

    fn row(buf: &PixelBuffer, y: u32) -> Vec<u32> {
        (0..buf.width()).map(|x| buf.pixel(x, y)).collect()
    }

    #[test]
    fn new_buffer_is_cleared() {
        let buf = buffer(4, 3);
        assert_eq!((buf.width(), buf.height()), (4, 3));
        assert!((0..3).all(|y| row(&buf, y) == [0; 4]));
    }

    #[test]
    fn fill_rect() {
        let mut buf = buffer(4, 4);
        buf.fill_rect(1, 1, 2, 2, 0xAB);
        assert_eq!(row(&buf, 0), [0, 0, 0, 0]);
        assert_eq!(row(&buf, 1), [0, 0xAB, 0xAB, 0]);
        assert_eq!(row(&buf, 2), [0, 0xAB, 0xAB, 0]);
        assert_eq!(row(&buf, 3), [0, 0, 0, 0]);
    }

    #[test]
    fn fill_rect_clips() {
        let mut buf = buffer(4, 4);
        buf.fill_rect(2, 3, 100, 100, 0x11);
        assert_eq!(row(&buf, 3), [0, 0, 0x11, 0x11]);
        buf.fill_rect(4, 0, 1, 1, 0x22);
        buf.fill_rect(0, 4, 1, 1, 0x22);
        assert!((0..3).all(|y| row(&buf, y) == [0; 4]));
    }

    #[test]
    fn draw_rect_outline() {
        let mut buf = buffer(5, 4);
        buf.draw_rect(0, 0, 4, 3, 0xCC);
        assert_eq!(row(&buf, 0), [0xCC, 0xCC, 0xCC, 0xCC, 0]);
        assert_eq!(row(&buf, 1), [0xCC, 0, 0, 0xCC, 0]);
        assert_eq!(row(&buf, 2), [0xCC, 0xCC, 0xCC, 0xCC, 0]);
        assert_eq!(row(&buf, 3), [0; 5]);
    }

    #[test]
    fn draw_rect_far_off_screen_draws_nothing() {
        let mut buf = buffer(4, 4);
        buf.draw_rect(0, u32::MAX, 4, 5, 0xCC);
        buf.draw_rect(u32::MAX, 0, 5, 4, 0xCC);
        assert!((0..4).all(|y| row(&buf, y) == [0; 4]));
    }

    #[test]
    fn draw_char_space_is_background() {
        let mut buf = buffer(8, 16);
        buf.draw_char(0, 0, b' ', 0xFF, 0x11);
        assert!((0..16).all(|y| row(&buf, y) == [0x11; 8]));
    }

    #[test]
    fn draw_char_a_matches_font() {
        let mut buf = buffer(8, 16);
        buf.draw_char(0, 0, b'A', 0xFF, 0x00);
        for (y, bits) in FONT[b'A' as usize].iter().enumerate() {
            for x in 0..8 {
                let expected = if bits & (0x80 >> x) != 0 { 0xFF } else { 0 };
                assert_eq!(buf.pixel(x, y as u32), expected);
            }
        }
        assert!(FONT[b'A' as usize].iter().any(|&bits| bits != 0));
    }

    #[test]
    fn draw_char_clips_at_edges() {
        let mut buf = buffer(4, 4);
        buf.draw_char(2, 2, 0xDB, 0x77, 0); // full block glyph
        assert_eq!(row(&buf, 3), [0, 0, 0x77, 0x77]);
    }

    #[test]
    fn scroll_moves_content_up() {
        let mut buf = buffer(4, 4);
        buf.fill_rect(0, 2, 4, 1, 0xDD);
        buf.scroll(1, 0x00);
        assert_eq!(row(&buf, 1), [0xDD; 4]);
        assert_eq!(row(&buf, 2), [0; 4]);
    }

    #[test]
    fn scroll_fills_new_rows() {
        let mut buf = buffer(4, 4);
        buf.fill_rect(0, 0, 4, 4, 0xFF);
        buf.scroll(2, 0xAA);
        assert_eq!(row(&buf, 0), [0xFF; 4]);
        assert_eq!(row(&buf, 1), [0xFF; 4]);
        assert_eq!(row(&buf, 2), [0xAA; 4]);
        assert_eq!(row(&buf, 3), [0xAA; 4]);
    }

    #[test]
    fn scroll_full_height_clears() {
        let mut buf = buffer(4, 4);
        buf.fill_rect(0, 0, 4, 4, 0xFF);
        buf.scroll(1, 0x00);
        buf.scroll(4, 0xAA);
        assert!((0..4).all(|y| row(&buf, y) == [0xAA; 4]));
    }

    #[test]
    fn scroll_wraps_around() {
        let mut buf = buffer(4, 4);
        for _ in 0..3 {
            buf.scroll(2, 0x00);
        }
        buf.fill_rect(0, 3, 1, 1, 0x77);
        assert_eq!(row(&buf, 3), [0x77, 0, 0, 0]);
        assert_eq!(buf.top, 2);
    }

    #[test]
    fn blit_copies_image() {
        let mut buf = buffer(4, 4);
        buf.blit(1, 1, 2, 2, &[0x11, 0x22, 0x33, 0x44]);
        assert_eq!(row(&buf, 1), [0, 0x11, 0x22, 0]);
        assert_eq!(row(&buf, 2), [0, 0x33, 0x44, 0]);
    }

    #[test]
    fn blit_clips_to_buffer_and_source() {
        let mut buf = buffer(4, 4);
        // 3x3 image at (2, 2): only a 2x2 corner is visible.
        buf.blit(2, 2, 3, 3, &[1, 2, 3, 4, 5, 6, 7, 8, 9]);
        assert_eq!(row(&buf, 2), [0, 0, 1, 2]);
        assert_eq!(row(&buf, 3), [0, 0, 4, 5]);
        // Claimed 2x4 image but only one row of pixels supplied.
        let mut buf = buffer(4, 4);
        buf.blit(0, 0, 2, 4, &[7, 8]);
        assert_eq!(row(&buf, 0), [7, 8, 0, 0]);
        assert_eq!(row(&buf, 1), [0; 4]);
    }

    #[test]
    fn copy_to_unwraps_rows_and_uses_stride() {
        let mut buf = buffer(2, 3);
        buf.fill_rect(0, 0, 2, 1, 1);
        buf.fill_rect(0, 1, 2, 1, 2);
        buf.fill_rect(0, 2, 2, 1, 3);
        buf.scroll(1, 9);
        let mut dst = [0u32; 9];
        buf.copy_to(&mut dst, 3);
        assert_eq!(dst, [2, 2, 0, 3, 3, 0, 9, 9, 0]);
    }

    #[test]
    fn empty_buffer_is_harmless() {
        let mut buf = buffer(0, 0);
        buf.fill_rect(0, 0, 1, 1, 1);
        buf.draw_char(0, 0, b'A', 1, 0);
        buf.scroll(1, 0);
        buf.blit(0, 0, 1, 1, &[1]);
        buf.copy_to(&mut [], 0);
    }
}
