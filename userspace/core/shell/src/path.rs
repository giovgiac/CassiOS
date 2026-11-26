// path.rs -- working directory paths
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::vec::Vec;

/// `input` as an absolute path: as is if it starts with '/', otherwise
/// relative to the directory `cwd`. Trailing slashes are dropped, so
/// "docs/" names the same directory as "docs".
pub fn resolve(cwd: &[u8], input: &[u8]) -> Vec<u8> {
    let mut out = if input.first() == Some(&b'/') {
        input.to_vec()
    } else {
        let mut out = cwd.to_vec();
        if cwd != b"/" {
            out.push(b'/');
        }
        out.extend_from_slice(input);
        out
    };
    while out.len() > 1 && out.ends_with(b"/") {
        out.pop();
    }
    out
}

/// Remove the last component of the absolute path `path` ("/" stays).
pub fn parent(path: &mut Vec<u8>) {
    match path.iter().rposition(|&c| c == b'/') {
        Some(0) | None => {
            path.clear();
            path.push(b'/');
        }
        Some(last) => path.truncate(last),
    }
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
    fn trailing_slashes_are_dropped() {
        assert_eq!(resolve(b"/", b"docs/"), b"/docs");
        assert_eq!(resolve(b"/home", b"docs//"), b"/home/docs");
        assert_eq!(resolve(b"/home", b"/"), b"/");
        assert_eq!(resolve(b"/home", b"//"), b"/");
        let mut cwd = resolve(b"/", b"docs/");
        parent(&mut cwd);
        assert_eq!(cwd, b"/");
    }

    #[test]
    fn long_paths_are_kept_whole() {
        let name = [b'n'; 200];
        let path = resolve(b"/dir", &name);
        assert_eq!(path.len(), 5 + 200);
    }

    #[test]
    fn parents() {
        let mut path = b"/".to_vec();
        parent(&mut path);
        assert_eq!(path, b"/");
        let mut path = b"/home".to_vec();
        parent(&mut path);
        assert_eq!(path, b"/");
        let mut path = b"/home/user".to_vec();
        parent(&mut path);
        assert_eq!(path, b"/home");
    }
}
