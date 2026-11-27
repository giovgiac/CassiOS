// args.rs -- command line splitting
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::vec::Vec;

/// Split `line` into words separated by spaces.
pub fn parse(line: &[u8]) -> Vec<&[u8]> {
    line.split(|&c| c == b' ')
        .filter(|word| !word.is_empty())
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
    fn every_word_is_kept() {
        let line = b"a b c d e f g h i j k l m n o p q r s";
        let args = parse(line);
        assert_eq!(args.len(), 19);
        assert_eq!(args[18], b"s");
    }
}
