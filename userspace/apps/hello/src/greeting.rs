// greeting.rs -- where the greeting goes on screen
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::gfx::{FONT_HEIGHT, FONT_WIDTH};

pub const TEXT: &[u8] = b"Hello, World!";

/// Pixel position of character `index` of TEXT on a `width` x `height`
/// screen: the text is centered on the middle character row.
pub fn position(width: u32, height: u32, index: usize) -> (u32, u32) {
    let half_cols = width / FONT_WIDTH / 2;
    let half_rows = height / FONT_HEIGHT / 2;
    let first = half_cols.saturating_sub(TEXT.len() as u32 / 2);
    ((first + index as u32) * FONT_WIDTH, half_rows * FONT_HEIGHT)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn centered_on_a_1024x768_screen() {
        // 128 columns and 48 rows: 13 characters start at column 58, row 24.
        assert_eq!(position(1024, 768, 0), (58 * 8, 24 * 16));
        assert_eq!(position(1024, 768, 12), (70 * 8, 24 * 16));
    }

    #[test]
    fn narrow_screens_start_at_the_left_edge() {
        assert_eq!(position(64, 32, 0), (0, 16));
        assert_eq!(position(0, 0, 3), (3 * 8, 0));
    }
}
