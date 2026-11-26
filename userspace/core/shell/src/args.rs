// args.rs -- command line splitting
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::vec::Vec;

/// Most words a command line is split into; later ones are dropped.
pub const MAX_ARGS: usize = 16;

/// Split `line` into words separated by spaces.
pub fn parse(line: &[u8]) -> Vec<&[u8]> {
    line.split(|&c| c == b' ')
        .filter(|word| !word.is_empty())
        .take(MAX_ARGS)
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn single_word() {
        assert_eq!(parse(b"help"), [b"help"]);
    }

    #[test]
    fn multiple_words() {
        assert_eq!(parse(b"write foo hello"), [&b"write"[..], b"foo", b"hello"]);
    }

    #[test]
    fn extra_spaces_are_skipped() {
        assert_eq!(parse(b"  ls"), [b"ls"]);
        assert_eq!(parse(b"ls   "), [b"ls"]);
        assert_eq!(parse(b"cat   a  b"), [&b"cat"[..], b"a", b"b"]);
    }

    #[test]
    fn empty() {
        assert!(parse(b"").is_empty());
        assert!(parse(b"    ").is_empty());
    }

    #[test]
    fn at_most_max_args() {
        let line = b"a b c d e f g h i j k l m n o p q r s";
        let args = parse(line);
        assert_eq!(args.len(), MAX_ARGS);
        assert_eq!(args[MAX_ARGS - 1], b"p");
    }
}
