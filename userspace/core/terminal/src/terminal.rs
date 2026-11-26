// terminal.rs -- text grid on top of the display
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Keeps a character grid mirroring the screen and draws characters as
// 8x16 glyphs. Scrolling is deferred until the next draw, so a run of
// newlines costs one display scroll. The cursor is only drawn around a
// flush, so the back buffer never holds it while text is written.

use alloc::vec::Vec;

use cassio::gfx::{Color, FONT_HEIGHT, FONT_WIDTH};

const FG: Color = 0x00AAAAAA;
const BG: Color = 0x00000000;
const BACKSPACE: u8 = 0x08;
const TAB: u8 = 0x09;
const NEWLINE: u8 = 0x0A;
const DELETE: u8 = 0x7F;

/// The drawing operations the terminal needs from the display.
pub trait Screen {
    fn fill_rect(&mut self, x: u32, y: u32, w: u32, h: u32, color: Color);
    fn draw_char(&mut self, x: u32, y: u32, ch: u8, fg: Color, bg: Color);
    /// Scroll the content up by `rows` pixels.
    fn scroll(&mut self, rows: u32, color: Color);
    fn flush(&mut self);
}

#[cfg(target_arch = "x86")]
impl Screen for cassio::display::Display {
    fn fill_rect(&mut self, x: u32, y: u32, w: u32, h: u32, color: Color) {
        cassio::display::Display::fill_rect(self, x, y, w, h, color);
    }
    fn draw_char(&mut self, x: u32, y: u32, ch: u8, fg: Color, bg: Color) {
        cassio::display::Display::draw_char(self, x, y, ch, fg, bg);
    }
    fn scroll(&mut self, rows: u32, color: Color) {
        cassio::display::Display::scroll(self, rows, color);
    }
    fn flush(&mut self) {
        cassio::display::Display::flush(self);
    }
}

/// A text terminal of `cols` x `rows` character cells.
pub struct Terminal<S: Screen> {
    screen: S,
    cols: u32,
    rows: u32,
    x: u32,
    y: u32,
    /// Characters on screen, row by row; empty if it could not be
    /// allocated (the terminal still draws, it only forgets what is shown).
    cells: Vec<u8>,
    /// Rows scrolled off but not yet applied to the screen and the grid.
    pending_scroll: u32,
}

impl<S: Screen> Terminal<S> {
    /// A terminal filling a `width` x `height` pixel screen.
    pub fn new(screen: S, width: u32, height: u32) -> Self {
        let (cols, rows) = (width / FONT_WIDTH, height / FONT_HEIGHT);
        let len = (cols * rows) as usize;
        let mut cells = Vec::new();
        if cells.try_reserve_exact(len).is_ok() {
            cells.resize(len, b' ');
        }
        Self {
            screen,
            cols,
            rows,
            x: 0,
            y: 0,
            cells,
            pending_scroll: 0,
        }
    }

    /// The cursor position (column, row).
    pub fn cursor(&self) -> (u32, u32) {
        (self.x, self.y)
    }

    /// Move the cursor without drawing.
    pub fn set_cursor(&mut self, col: u32, row: u32) {
        self.x = col;
        self.y = row;
    }

    /// Write one character at the cursor, interpreting newline, backspace,
    /// tab and DEL, and wrapping and scrolling as needed.
    pub fn putchar(&mut self, ch: u8) {
        if self.cols == 0 || self.rows == 0 {
            return;
        }
        match ch {
            NEWLINE => {
                self.y += 1;
                self.x = 0;
            }
            BACKSPACE => {
                if self.x > 0 {
                    self.x -= 1;
                } else if self.y > 0 {
                    self.y -= 1;
                    self.x = self.cols - 1;
                }
                self.clear_cell(self.x, self.y);
            }
            DELETE => self.clear_cell(self.x, self.y),
            TAB => {
                let next = ((self.x + 8) & !7).min(self.cols);
                while self.x < next {
                    self.clear_cell(self.x, self.y);
                    self.x += 1;
                }
            }
            _ => {
                self.draw_glyph(ch, self.x, self.y, FG, BG);
                self.x += 1;
            }
        }
        if self.x >= self.cols {
            self.y += 1;
            self.x = 0;
        }
        if self.y >= self.rows {
            self.pending_scroll += self.y - self.rows + 1;
            self.y = self.rows - 1;
        }
    }

    /// Blank the screen and move the cursor home.
    pub fn clear(&mut self) {
        self.pending_scroll = 0;
        self.cells.fill(b' ');
        self.screen
            .fill_rect(0, 0, self.cols * FONT_WIDTH, self.rows * FONT_HEIGHT, BG);
        self.x = 0;
        self.y = 0;
    }

    /// Show everything written so far, with the cursor drawn as an
    /// inverted cell. The cursor is removed from the back buffer again
    /// afterwards, so later writes never have to account for it.
    pub fn present(&mut self) {
        self.flush_scroll();
        let ch = self.cell(self.x, self.y);
        self.draw_at(ch, self.x, self.y, BG, FG);
        self.screen.flush();
        self.draw_at(ch, self.x, self.y, FG, BG);
    }

    fn cell(&self, col: u32, row: u32) -> u8 {
        self.cell_index(col, row)
            .and_then(|i| self.cells.get(i).copied())
            .unwrap_or(b' ')
    }

    fn cell_index(&self, col: u32, row: u32) -> Option<usize> {
        (col < self.cols && row < self.rows).then(|| (row * self.cols + col) as usize)
    }

    fn draw_glyph(&mut self, ch: u8, col: u32, row: u32, fg: Color, bg: Color) {
        self.flush_scroll();
        if let Some(cell) = self
            .cell_index(col, row)
            .and_then(|i| self.cells.get_mut(i))
        {
            *cell = ch;
        }
        self.draw_at(ch, col, row, fg, bg);
    }

    fn draw_at(&mut self, ch: u8, col: u32, row: u32, fg: Color, bg: Color) {
        self.screen
            .draw_char(col * FONT_WIDTH, row * FONT_HEIGHT, ch, fg, bg);
    }

    fn clear_cell(&mut self, col: u32, row: u32) {
        self.flush_scroll();
        if let Some(cell) = self
            .cell_index(col, row)
            .and_then(|i| self.cells.get_mut(i))
        {
            *cell = b' ';
        }
        self.screen.fill_rect(
            col * FONT_WIDTH,
            row * FONT_HEIGHT,
            FONT_WIDTH,
            FONT_HEIGHT,
            BG,
        );
    }

    /// Apply deferred scrolling to the screen and the character grid.
    fn flush_scroll(&mut self) {
        if self.pending_scroll == 0 {
            return;
        }
        self.screen.scroll(self.pending_scroll * FONT_HEIGHT, BG);
        let cols = self.cols as usize;
        let shift = (self.pending_scroll as usize * cols).min(self.cells.len());
        self.cells.copy_within(shift.., 0);
        let len = self.cells.len();
        self.cells[len - shift..].fill(b' ');
        self.pending_scroll = 0;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Debug, PartialEq)]
    enum Op {
        Fill(u32, u32, u32, u32),
        Char(u32, u32, u8, Color),
        Scroll(u32),
        Flush,
    }

    #[derive(Default)]
    struct Fake(Vec<Op>);

    impl Screen for &mut Fake {
        fn fill_rect(&mut self, x: u32, y: u32, w: u32, h: u32, _: Color) {
            self.0.push(Op::Fill(x, y, w, h));
        }
        fn draw_char(&mut self, x: u32, y: u32, ch: u8, fg: Color, _: Color) {
            self.0.push(Op::Char(x, y, ch, fg));
        }
        fn scroll(&mut self, rows: u32, _: Color) {
            self.0.push(Op::Scroll(rows));
        }
        fn flush(&mut self) {
            self.0.push(Op::Flush);
        }
    }

    /// A 4x3-cell terminal (32x48 pixels).
    fn terminal(fake: &mut Fake) -> Terminal<&mut Fake> {
        Terminal::new(fake, 4 * FONT_WIDTH, 3 * FONT_HEIGHT)
    }

    fn write<S: Screen>(terminal: &mut Terminal<S>, text: &[u8]) {
        for &ch in text {
            terminal.putchar(ch);
        }
    }

    fn grid<S: Screen>(terminal: &Terminal<S>) -> Vec<u8> {
        terminal.cells.clone()
    }

    #[test]
    fn size_from_pixels() {
        let mut fake = Fake::default();
        let t = Terminal::new(&mut fake, 1024, 768);
        assert_eq!((t.cols, t.rows, t.cells.len()), (128, 48, 128 * 48));
    }

    #[test]
    fn characters_are_drawn_and_recorded() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"hi");
        assert_eq!(t.cursor(), (2, 0));
        assert_eq!(&grid(&t)[..4], b"hi  ");
        drop(t);
        assert_eq!(fake.0, [Op::Char(0, 0, b'h', FG), Op::Char(8, 0, b'i', FG)]);
    }

    #[test]
    fn newline_and_wrap() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"a\nbcdef");
        assert_eq!(t.cursor(), (1, 2));
        assert_eq!(grid(&t), b"a   bcdef   ");
    }

    #[test]
    fn backspace_moves_back_and_clears() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"ab\x08");
        assert_eq!(t.cursor(), (1, 0));
        assert_eq!(&grid(&t)[..4], b"a   ");
        drop(t);
        assert_eq!(fake.0.last(), Some(&Op::Fill(8, 0, 8, 16)));
    }

    #[test]
    fn backspace_at_line_start_goes_to_previous_line() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"abcd\x08");
        assert_eq!(t.cursor(), (3, 0));
        assert_eq!(&grid(&t)[..4], b"abc ");
    }

    #[test]
    fn backspace_at_home_stays() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"\x08");
        assert_eq!(t.cursor(), (0, 0));
    }

    #[test]
    fn delete_clears_without_moving() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"ab");
        t.set_cursor(0, 0);
        write(&mut t, b"\x7F");
        assert_eq!(t.cursor(), (0, 0));
        assert_eq!(&grid(&t)[..4], b" b  ");
    }

    #[test]
    fn tab_stops_at_eight_or_line_end() {
        let mut fake = Fake::default();
        let mut t = Terminal::new(&mut fake, 20 * FONT_WIDTH, FONT_HEIGHT * 2);
        write(&mut t, b"a\t");
        assert_eq!(t.cursor(), (8, 0));
        write(&mut t, b"\t\t");
        // 8 -> 16, then 16 -> 20 (line end), which wraps.
        assert_eq!(t.cursor(), (0, 1));
    }

    #[test]
    fn scrolling_is_deferred_until_the_next_draw() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"a\nb\nc\n\n");
        assert_eq!(t.cursor(), (0, 2));
        assert_eq!(t.pending_scroll, 2);
        write(&mut t, b"d");
        assert_eq!(grid(&t), b"c       d   ");
        drop(t);
        // One scroll of both rows, then the glyph.
        assert_eq!(
            &fake.0[3..],
            [Op::Scroll(2 * FONT_HEIGHT), Op::Char(0, 32, b'd', FG)]
        );
    }

    #[test]
    fn scrolling_more_than_the_screen_blanks_the_grid() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"abc\n\n\n\n\n\nx");
        assert_eq!(grid(&t), b"        x   ");
    }

    #[test]
    fn clear_blanks_everything() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"abc\n\n\n\n");
        t.clear();
        assert_eq!((t.cursor(), t.pending_scroll), ((0, 0), 0));
        assert_eq!(grid(&t), [b' '; 12]);
        drop(t);
        assert_eq!(fake.0.last(), Some(&Op::Fill(0, 0, 32, 48)));
    }

    #[test]
    fn present_draws_and_removes_the_cursor() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"ab");
        t.set_cursor(1, 0);
        t.present();
        drop(t);
        assert_eq!(
            &fake.0[2..],
            [
                Op::Char(8, 0, b'b', BG),
                Op::Flush,
                Op::Char(8, 0, b'b', FG)
            ]
        );
    }

    #[test]
    fn present_applies_pending_scroll_first() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        write(&mut t, b"\n\n\n");
        t.present();
        drop(t);
        assert_eq!(fake.0[0], Op::Scroll(FONT_HEIGHT));
    }

    #[test]
    fn cursor_outside_the_grid_draws_blank() {
        let mut fake = Fake::default();
        let mut t = terminal(&mut fake);
        t.set_cursor(10, 10);
        t.present();
        drop(t);
        assert_eq!(fake.0[0], Op::Char(80, 160, b' ', BG));
    }

    #[test]
    fn zero_size_terminal_ignores_writes() {
        let mut fake = Fake::default();
        let mut t = Terminal::new(&mut fake, 0, 0);
        write(&mut t, b"abc\n\x08\t");
        t.clear();
        t.present();
        assert_eq!(t.cursor(), (0, 0));
    }
}
