// commands/system.rs -- help, clear, echo, mem, ps, uptime, reboot,
// shutdown, exec
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::format;
use alloc::string::String;

use cassio::os::{MemInfo, ProcEntry, TICK_FREQUENCY};

#[cfg(target_arch = "x86")]
const HELP: &[u8] = b"Available commands:
  help        - Show this help
  clear       - Clear the screen
  echo        - Print text
  mem         - Show memory statistics
  ps          - List running processes
  uptime      - Show time since boot
  reboot      - Reboot the system
  shutdown    - Halt the system
  ls          - List directory
  cd          - Change directory
  pwd         - Print working directory
  mkdir       - Create directory
  rmdir       - Remove directory
  touch       - Create empty file
  rm          - Remove file
  cat         - Print file contents
  write       - Write text to file
  exec        - Execute a program
";

pub fn mem_text(mem: &MemInfo) -> String {
    let line = |label: &str, frames: u32| {
        format!("  {label} {} KiB ({frames} frames)\n", frames as u64 * 4)
    };
    format!(
        "Physical memory:\n{}{}{}",
        line("Total:", mem.total),
        line("Used: ", mem.used),
        line("Free: ", mem.free)
    )
}

pub fn uptime_text(ticks: u32) -> String {
    let seconds = ticks / TICK_FREQUENCY;
    let ms = ticks % TICK_FREQUENCY * 1000 / TICK_FREQUENCY;
    format!("Up {seconds}.{ms:03}s\n")
}

pub fn state_name(state: u32) -> &'static str {
    match state {
        1 => "Ready",
        2 => "Running",
        3 => "SendBlocked",
        4 => "ReceiveBlocked",
        5 => "WaitBlocked",
        _ => "Unknown",
    }
}

/// One `ps` row for `proc`, registered as `name` (if at all).
pub fn ps_line(proc: &ProcEntry, name: Option<&[u8]>) -> String {
    let name = name
        .and_then(|n| core::str::from_utf8(n).ok())
        .unwrap_or("<unknown>");
    format!(
        "{:3}  {:<9}  {:<14}  {} KB\n",
        proc.pid,
        name,
        state_name(proc.state),
        proc.heap_size / 1024
    )
}

#[cfg(target_arch = "x86")]
mod imp {
    use alloc::vec::Vec;

    use cassio::os::{self, ProcEntry};
    use cassio::{ns, vfs};

    use super::*;
    use crate::shell::Shell;

    /// Largest program `exec` loads.
    const MAX_ELF_SIZE: usize = 64 * 1024;

    /// Processes and names `ps` asks for.
    const PS_MAX: usize = 16;

    const PS_HEADER: &[u8] = b"PID  NAME       STATE           HEAP\n";

    impl Shell {
        pub(crate) fn help(&self) {
            self.print(HELP);
        }

        pub(crate) fn clear(&self) {
            self.terminal.clear();
            self.terminal.flush();
        }

        pub(crate) fn echo(&self, args: &[&[u8]]) {
            self.print(&crate::commands::file::join(&args[1..]));
            self.terminal.putchar(b'\n');
        }

        pub(crate) fn mem(&self) {
            self.print(mem_text(&os::mem_info()).as_bytes());
        }

        pub(crate) fn ps(&self) {
            let mut procs = alloc::vec![ProcEntry::default(); PS_MAX];
            let count = os::proc_list(&mut procs);
            let mut names = alloc::vec![ns::Entry::default(); PS_MAX];
            let named = ns::list_all(&mut names);
            self.print(PS_HEADER);
            for proc in &procs[..count] {
                let name = names[..named]
                    .iter()
                    .find(|entry| entry.pid == proc.pid)
                    .map(ns::Entry::name);
                self.print(ps_line(proc, name).as_bytes());
            }
        }

        pub(crate) fn uptime(&self) {
            self.print(uptime_text(os::uptime()).as_bytes());
        }

        pub(crate) fn reboot(&self) {
            self.print(b"Rebooting...\n");
            os::reboot();
        }

        pub(crate) fn shutdown(&self) {
            self.print(b"Shutting down...\n");
            os::shutdown();
        }

        pub(crate) fn exec(&mut self, args: &[&[u8]]) {
            let Some(&input) = args.get(1) else {
                return self.print(b"exec: usage: exec <path>\n");
            };
            let Some(handle) = self.vfs.open(&self.path(input), false) else {
                return self.fail(b"exec: no such file: ", input);
            };
            // The whole file, on the heap: the user stack is a single page.
            let mut elf = Vec::new();
            let mut chunk = alloc::vec![0u8; vfs::BUF_SIZE];
            let mut too_large = false;
            loop {
                let n = self.vfs.read(handle, elf.len() as u32, &mut chunk);
                if n == 0 {
                    break;
                }
                if elf.len() + n > MAX_ELF_SIZE {
                    too_large = true;
                    break;
                }
                elf.extend_from_slice(&chunk[..n]);
            }
            self.vfs.close(handle);
            drop(chunk);

            if too_large {
                return self.fail(b"exec: file too large: ", input);
            }
            if elf.is_empty() {
                return self.print(b"exec: empty file\n");
            }
            let pid = os::exec(&elf);
            drop(elf);
            if pid == 0 {
                return self.print(b"exec: failed to load program\n");
            }
            os::waitpid(pid);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn mem_lines() {
        let mem = MemInfo {
            total: 1024,
            used: 24,
            free: 1000,
        };
        assert_eq!(
            mem_text(&mem),
            "Physical memory:\n  Total: 4096 KiB (1024 frames)\n  Used:  96 KiB (24 frames)\n  Free:  4000 KiB (1000 frames)\n"
        );
    }

    #[test]
    fn uptime_in_seconds_and_milliseconds() {
        assert_eq!(uptime_text(0), "Up 0.000s\n");
        assert_eq!(uptime_text(1234), "Up 1.234s\n");
        assert_eq!(uptime_text(60_005), "Up 60.005s\n");
        // Past 71 minutes, where ticks * 1000 would overflow a u32.
        assert_eq!(uptime_text(5_000_000), "Up 5000.000s\n");
        assert_eq!(uptime_text(u32::MAX), "Up 4294967.295s\n");
    }

    #[test]
    fn ps_rows() {
        let proc = ProcEntry {
            pid: 3,
            state: 4,
            heap_size: 8192,
        };
        assert_eq!(
            ps_line(&proc, Some(b"kbd")),
            "  3  kbd        ReceiveBlocked  8 KB\n"
        );
        let proc = ProcEntry {
            pid: 12,
            state: 9,
            heap_size: 0,
        };
        assert_eq!(
            ps_line(&proc, None),
            " 12  <unknown>  Unknown         0 KB\n"
        );
    }

    #[test]
    fn state_names() {
        let names: alloc::vec::Vec<_> = (0..=6).map(state_name).collect();
        assert_eq!(
            names,
            [
                "Unknown",
                "Ready",
                "Running",
                "SendBlocked",
                "ReceiveBlocked",
                "WaitBlocked",
                "Unknown"
            ]
        );
    }
}
