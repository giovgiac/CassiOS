// commands/navigation.rs -- ls, cd, pwd
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

#[cfg(target_arch = "x86")]
mod imp {
    use cassio::vfs::Stat;

    use crate::shell::Shell;

    impl Shell {
        pub(crate) fn ls(&mut self, args: &[&[u8]]) {
            let path = match args.get(1) {
                Some(input) => self.path(input),
                None => self.cwd.clone(),
            };
            if self.vfs.stat(&path) != Stat::Directory {
                return self.fail(b"ls: no such directory: ", &path);
            }
            let mut index = 0;
            while let Some(name) = self.vfs.list(&path, index) {
                self.print(&name);
                self.terminal.putchar(b'\n');
                index += 1;
            }
        }

        pub(crate) fn cd(&mut self, args: &[&[u8]]) {
            let Some(&input) = args.get(1) else {
                self.cwd = b"/".to_vec();
                return;
            };
            if input == b".." {
                crate::path::parent(&mut self.cwd);
                return;
            }
            let path = self.path(input);
            if self.vfs.stat(&path) != Stat::Directory {
                return self.fail(b"cd: no such directory: ", input);
            }
            self.cwd = path;
        }

        pub(crate) fn pwd(&self) {
            self.print(&self.cwd);
            self.terminal.putchar(b'\n');
        }
    }
}
