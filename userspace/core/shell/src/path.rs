// path.rs -- working directory paths
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::vec::Vec;

/// `input` as a normalized absolute path: taken as is if it starts with
/// '/', otherwise relative to the directory `cwd`. "." components and
/// empty ones (repeated or trailing slashes) are dropped, and ".." removes
/// the component before it, stopping at the root.
pub fn resolve(cwd: &[u8], input: &[u8]) -> Vec<u8> {
    let base: &[u8] = if input.first() == Some(&b'/') {
        b""
    } else {
        cwd
    };
    let mut parts: Vec<&[u8]> = Vec::new();
    for part in base
        .split(|&c| c == b'/')
        .chain(input.split(|&c| c == b'/'))
    {
        match part {
            b"" | b"." => {}
            b".." => {
                parts.pop();
            }
            _ => parts.push(part),
        }
    }
    let mut out = Vec::new();
    for part in parts {
        out.push(b'/');
        out.extend_from_slice(part);
    }
    if out.is_empty() {
        out.push(b'/');
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn absolute_paths_ignore_the_cwd() {
        assert_eq!(resolve(b"/", b"/foo"), b"/foo");
        assert_eq!(resolve(b"/home", b"/bar"), b"/bar");
    }

    #[test]
    fn relative_paths() {
        assert_eq!(resolve(b"/", b"foo"), b"/foo");
        assert_eq!(resolve(b"/home", b"foo"), b"/home/foo");
    }

    #[test]
    fn slashes_are_collapsed() {
        assert_eq!(resolve(b"/", b"docs/"), b"/docs");
        assert_eq!(resolve(b"/home", b"docs//x/"), b"/home/docs/x");
        assert_eq!(resolve(b"/home", b"/"), b"/");
        assert_eq!(resolve(b"/home", b"//"), b"/");
    }

    #[test]
    fn dots_are_resolved() {
        assert_eq!(resolve(b"/", b".."), b"/");
        assert_eq!(resolve(b"/home", b".."), b"/");
        assert_eq!(resolve(b"/home/user", b".."), b"/home");
        assert_eq!(resolve(b"/", b"docs/.."), b"/");
        assert_eq!(resolve(b"/home", b"./a/../b/."), b"/home/b");
        assert_eq!(resolve(b"/home", b"../../../etc"), b"/etc");
    }

    #[test]
    fn long_paths_are_kept_whole() {
        let name = [b'n'; 200];
        let path = resolve(b"/dir", &name);
        assert_eq!(path.len(), 5 + 200);
    }
}
