// shell.rs -- prompt, line editing and command dispatch
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Output goes to the terminal by notify; only the prompt (which needs the
// cursor position) and redraws (which must follow earlier output) wait
// for the terminal.

use alloc::vec::Vec;

use cassio::kbd;
use cassio::terminal::Terminal;
use cassio::vfs::Vfs;

use crate::args;
use crate::line::{Change, Line};

pub struct Shell {
    pub(crate) terminal: Terminal,
    pub(crate) vfs: Vfs,
    pub(crate) cwd: Vec<u8>,
    line: Line,
    /// Where the line starts on screen, after the prompt.
    prompt: (u32, u32),
}

impl Shell {
    pub fn connect() -> Self {
        Self {
            terminal: Terminal::connect(),
            vfs: Vfs::connect(),
            cwd: b"/".to_vec(),
            line: Line::new(),
            prompt: (0, 0),
        }
    }

    pub fn print(&self, text: &[u8]) {
        self.terminal.write(text);
    }

    /// Print the prompt at the start of a row, so the line always has the
    /// room MAX_INPUT assumes.
    pub fn prompt(&mut self) {
        if self.terminal.cursor().0 != 0 {
            self.terminal.putchar(b'\n');
        }
        self.print(b"$ ");
        self.terminal.flush();
        self.prompt = self.terminal.cursor();
    }

    /// Handle one key from the keyboard.
    pub fn key(&mut self, key: u8) {
        if key == kbd::ENTER {
            self.terminal.putchar(b'\n');
            self.execute();
            self.line.clear();
            self.prompt();
            return;
        }
        match self.line.key(key) {
            Change::None => return,
            Change::Echo(ch) => self.terminal.putchar(ch),
            Change::EraseLast => self.print(b"\x08 \x08"),
            Change::Redraw => self.redraw(),
            Change::Move => self.place_cursor(),
        }
        self.terminal.flush();
    }

    /// Rewrite the line from the prompt, blank the character a longer line
    /// left behind, and put the cursor back.
    fn redraw(&self) {
        let (col, row) = self.prompt;
        self.terminal.set_cursor(col, row);
        self.print(self.line.text());
        self.terminal.putchar(b' ');
        self.place_cursor();
    }

    fn place_cursor(&self) {
        let (col, row) = self.prompt;
        self.terminal
            .set_cursor(col + self.line.cursor() as u32, row);
    }

    fn execute(&mut self) {
        let text = self.line.text().to_vec();
        let args = args::parse(&text);
        let Some(&command) = args.first() else {
            return;
        };
        match command {
            b"help" => self.help(),
            b"clear" => self.clear(),
            b"echo" => self.echo(&args),
            b"mem" => self.mem(),
            b"ps" => self.ps(),
            b"uptime" => self.uptime(),
            b"reboot" => self.reboot(),
            b"shutdown" => self.shutdown(),
            b"ls" => self.ls(&args),
            b"cd" => self.cd(&args),
            b"pwd" => self.pwd(),
            b"mkdir" => self.mkdir(&args),
            b"rmdir" => self.rmdir(&args),
            b"touch" => self.touch(&args),
            b"rm" => self.rm(&args),
            b"cat" => self.cat(&args),
            b"write" => self.write(&args),
            b"exec" => self.exec(&args),
            _ => self.fail(b"Unknown command: ", command),
        }
    }

    /// Print `message`, then `what` and a newline.
    pub(crate) fn fail(&self, message: &[u8], what: &[u8]) {
        self.print(message);
        self.print(what);
        self.terminal.putchar(b'\n');
    }

    /// The absolute path for argument `input`.
    pub(crate) fn path(&self, input: &[u8]) -> Vec<u8> {
        crate::path::resolve(&self.cwd, input)
    }
}
