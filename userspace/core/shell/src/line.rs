// line.rs -- line editor
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Holds the line being typed and the cursor within it. Each edit reports
// what the screen needs, so the shell can draw as little as possible.

use alloc::vec::Vec;

use cassio::kbd;

/// Longest line. Redrawing assumes the line stays on the prompt's row, so
/// it must fit an 80-column screen after the "$ " prompt.
pub const MAX_INPUT: usize = 78;

/// How the screen must change after an edit.
#[derive(Debug, PartialEq, Eq)]
pub enum Change {
    /// Nothing changed.
    None,
    /// A character was added at the end of the line: print it.
    Echo(u8),
    /// The last character was removed while the cursor was at the end.
    EraseLast,
    /// Text changed before the end: redraw the line and place the cursor.
    Redraw,
    /// Only the cursor moved.
    Move,
}

pub struct Line {
    text: Vec<u8>,
    cursor: usize,
}

impl Line {
    pub fn new() -> Self {
        Self {
            text: Vec::with_capacity(MAX_INPUT),
            cursor: 0,
        }
    }

    pub fn text(&self) -> &[u8] {
        &self.text
    }

    pub fn cursor(&self) -> usize {
        self.cursor
    }

    pub fn clear(&mut self) {
        self.text.clear();
        self.cursor = 0;
    }

    /// Apply an editing key: a printable character, backspace, delete or
    /// a left or right arrow. Other keys change nothing.
    pub fn key(&mut self, key: u8) -> Change {
        match key {
            kbd::BACKSPACE => self.backspace(),
            kbd::DELETE => self.delete(),
            kbd::LEFT_ARROW if self.cursor > 0 => {
                self.cursor -= 1;
                Change::Move
            }
            kbd::RIGHT_ARROW if self.cursor < self.text.len() => {
                self.cursor += 1;
                Change::Move
            }
            0x20..=0x7E if self.text.len() < MAX_INPUT => {
                self.text.insert(self.cursor, key);
                self.cursor += 1;
                if self.cursor == self.text.len() {
                    Change::Echo(key)
                } else {
                    Change::Redraw
                }
            }
            _ => Change::None,
        }
    }

    fn backspace(&mut self) -> Change {
        if self.cursor == 0 {
            return Change::None;
        }
        self.cursor -= 1;
        self.text.remove(self.cursor);
        if self.cursor == self.text.len() {
            Change::EraseLast
        } else {
            Change::Redraw
        }
    }

    fn delete(&mut self) -> Change {
        if self.cursor == self.text.len() {
            return Change::None;
        }
        self.text.remove(self.cursor);
        Change::Redraw
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn typed(text: &[u8]) -> Line {
        let mut line = Line::new();
        for &c in text {
            line.key(c);
        }
        line
    }

    #[test]
    fn typing_at_the_end_echoes() {
        let mut line = Line::new();
        assert_eq!(line.key(b'l'), Change::Echo(b'l'));
        assert_eq!(line.key(b's'), Change::Echo(b's'));
        assert_eq!((line.text(), line.cursor()), (&b"ls"[..], 2));
    }

    #[test]
    fn typing_in_the_middle_redraws() {
        let mut line = typed(b"lx");
        assert_eq!(line.key(kbd::LEFT_ARROW), Change::Move);
        assert_eq!(line.key(b's'), Change::Redraw);
        assert_eq!((line.text(), line.cursor()), (&b"lsx"[..], 2));
    }

    #[test]
    fn backspace() {
        let mut line = typed(b"abc");
        assert_eq!(line.key(kbd::BACKSPACE), Change::EraseLast);
        assert_eq!(line.text(), b"ab");
        line.key(kbd::LEFT_ARROW);
        assert_eq!(line.key(kbd::BACKSPACE), Change::Redraw);
        assert_eq!((line.text(), line.cursor()), (&b"b"[..], 0));
        assert_eq!(line.key(kbd::BACKSPACE), Change::None);
    }

    #[test]
    fn delete() {
        let mut line = typed(b"abc");
        assert_eq!(line.key(kbd::DELETE), Change::None);
        line.key(kbd::LEFT_ARROW);
        line.key(kbd::LEFT_ARROW);
        assert_eq!(line.key(kbd::DELETE), Change::Redraw);
        assert_eq!((line.text(), line.cursor()), (&b"ac"[..], 1));
    }

    #[test]
    fn arrows_stay_within_the_line() {
        let mut line = typed(b"a");
        assert_eq!(line.key(kbd::RIGHT_ARROW), Change::None);
        assert_eq!(line.key(kbd::LEFT_ARROW), Change::Move);
        assert_eq!(line.key(kbd::LEFT_ARROW), Change::None);
        assert_eq!(line.cursor(), 0);
    }

    #[test]
    fn line_length_is_capped() {
        let mut line = typed(&[b'x'; MAX_INPUT]);
        assert_eq!(line.key(b'y'), Change::None);
        assert_eq!(line.text().len(), MAX_INPUT);
    }

    #[test]
    fn other_keys_are_ignored() {
        let mut line = typed(b"a");
        for key in [0, kbd::TAB, kbd::ESCAPE, kbd::F1, kbd::ENTER] {
            assert_eq!(line.key(key), Change::None);
        }
        assert_eq!(line.text(), b"a");
        line.clear();
        assert_eq!((line.text(), line.cursor()), (&b""[..], 0));
    }
}
