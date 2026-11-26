// commands/file.rs -- mkdir, rmdir, touch, rm, cat, write
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::vec::Vec;

/// The words of `args` joined by single spaces.
pub fn join(args: &[&[u8]]) -> Vec<u8> {
    args.join(&b' ')
}

#[cfg(target_arch = "x86")]
mod imp {
    use cassio::vfs::BUF_SIZE;

    use crate::shell::Shell;

    impl Shell {
        pub(crate) fn mkdir(&mut self, args: &[&[u8]]) {
            let Some(&input) = args.get(1) else {
                return self.print(b"mkdir: usage: mkdir <path>\n");
            };
            if !self.vfs.mkdir(&self.path(input)) {
                self.fail(b"mkdir: cannot create directory: ", input);
            }
        }

        pub(crate) fn rmdir(&mut self, args: &[&[u8]]) {
            let Some(&input) = args.get(1) else {
                return self.print(b"rmdir: usage: rmdir <path>\n");
            };
            if !self.vfs.remove(&self.path(input)) {
                self.fail(b"rmdir: cannot remove: ", input);
            }
        }

        pub(crate) fn touch(&mut self, args: &[&[u8]]) {
            let Some(&input) = args.get(1) else {
                return self.print(b"touch: usage: touch <path>\n");
            };
            match self.vfs.open(&self.path(input), true) {
                Some(handle) => {
                    self.vfs.close(handle);
                }
                None => self.fail(b"touch: cannot create file: ", input),
            }
        }

        pub(crate) fn rm(&mut self, args: &[&[u8]]) {
            let Some(&input) = args.get(1) else {
                return self.print(b"rm: usage: rm <path>\n");
            };
            if !self.vfs.remove(&self.path(input)) {
                self.fail(b"rm: cannot remove: ", input);
            }
        }

        pub(crate) fn cat(&mut self, args: &[&[u8]]) {
            let Some(&input) = args.get(1) else {
                return self.print(b"cat: usage: cat <filename>\n");
            };
            let Some(handle) = self.vfs.open(&self.path(input), false) else {
                return self.fail(b"cat: no such file: ", input);
            };
            // On the heap: the user stack is a single page.
            let mut buf = alloc::vec![0u8; BUF_SIZE];
            let mut offset = 0;
            loop {
                let n = self.vfs.read(handle, offset, &mut buf);
                if n == 0 {
                    break;
                }
                self.print(&buf[..n]);
                offset += n as u32;
            }
            self.vfs.close(handle);
            self.terminal.putchar(b'\n');
        }

        pub(crate) fn write(&mut self, args: &[&[u8]]) {
            if args.len() < 3 {
                return self.print(b"write: usage: write <filename> <text>\n");
            }
            let input = args[1];
            let Some(handle) = self.vfs.open(&self.path(input), true) else {
                return self.fail(b"write: cannot create file: ", input);
            };
            if !self.vfs.write(handle, &super::join(&args[2..])) {
                self.fail(b"write: cannot write: ", input);
            }
            self.vfs.close(handle);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn join_words() {
        assert_eq!(join(&[b"hello"]), b"hello");
        assert_eq!(join(&[&b"hello"[..], b"big", b"world"]), b"hello big world");
    }
}
