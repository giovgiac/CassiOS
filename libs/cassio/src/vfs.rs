// vfs.rs -- VFS service client
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Paths are sent NUL-terminated as request data. Files are opened by
// handle, read at an offset, written whole, and closed when done.

use alloc::vec::Vec;

use crate::ipc::{self, Message, message_type};
use crate::ns;

/// Longest name a directory entry can have.
pub const NAME_MAX: usize = 255;

/// Largest read or write a single request carries.
pub const BUF_SIZE: usize = 4096;

/// What a path refers to.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Stat {
    Missing,
    File,
    Directory,
}

/// A connection to the VFS service.
pub struct Vfs {
    pid: u32,
}

impl Vfs {
    /// Connect to the VFS service, retrying the lookup a bounded number of
    /// times while it starts. Without it, every request fails.
    pub fn connect() -> Self {
        Self {
            pid: ns::connect(b"vfs"),
        }
    }

    pub fn mkdir(&self, path: &[u8]) -> bool {
        self.path_request(message_type::VFS_MKDIR, 0, path) == Some(0)
    }

    /// Remove a file or an empty directory.
    pub fn remove(&self, path: &[u8]) -> bool {
        self.path_request(message_type::VFS_REMOVE, 0, path) == Some(0)
    }

    /// Open the file at `path`, creating it if `create` is set.
    pub fn open(&self, path: &[u8], create: bool) -> Option<u32> {
        self.path_request(message_type::VFS_OPEN, create as u32, path)
            .filter(|&handle| handle != 0)
    }

    pub fn close(&self, handle: u32) -> bool {
        let mut msg = Message {
            kind: message_type::VFS_CLOSE,
            arg1: handle,
            ..Message::default()
        };
        ipc::send(self.pid, &mut msg, &[]) >= 0 && msg.arg1 == 0
    }

    /// Read up to `buf.len()` bytes (at most BUF_SIZE) from `offset`;
    /// returns how many were read, 0 at the end of the file or on error.
    pub fn read(&self, handle: u32, offset: u32, buf: &mut [u8]) -> usize {
        let len = buf.len().min(BUF_SIZE);
        let mut msg = Message {
            kind: message_type::VFS_READ,
            arg1: handle,
            arg2: offset,
            arg3: len as u32,
            ..Message::default()
        };
        if ipc::send_into(self.pid, &mut msg, &mut buf[..len]) < 0 {
            return 0;
        }
        (msg.arg1 as usize).min(len)
    }

    /// Replace the file's contents with `data` (at most BUF_SIZE bytes).
    pub fn write(&self, handle: u32, data: &[u8]) -> bool {
        let mut msg = Message {
            kind: message_type::VFS_WRITE,
            arg1: handle,
            arg2: data.len() as u32,
            ..Message::default()
        };
        ipc::send(self.pid, &mut msg, data) >= 0 && msg.arg1 == 0
    }

    pub fn stat(&self, path: &[u8]) -> Stat {
        match self.path_request(message_type::VFS_STAT, 0, path) {
            Some(1) => Stat::File,
            Some(2) => Stat::Directory,
            _ => Stat::Missing,
        }
    }

    /// Name of the `index`th entry of the directory at `path`.
    pub fn list(&self, path: &[u8], index: u32) -> Option<Vec<u8>> {
        // The path goes out in the buffer the name comes back in.
        let mut buf = alloc::vec![0u8; (path.len() + 1).max(NAME_MAX + 1)];
        buf[..path.len()].copy_from_slice(path);
        let mut msg = Message {
            kind: message_type::VFS_LIST,
            arg1: index,
            ..Message::default()
        };
        if ipc::send_into(self.pid, &mut msg, &mut buf) < 0 || msg.arg1 == 0 {
            return None;
        }
        let len = buf.iter().position(|&c| c == 0).unwrap_or(buf.len());
        buf.truncate(len);
        Some(buf)
    }

    /// Send a request whose data is `path`, NUL-terminated; returns the
    /// reply's arg1, or None if the service cannot be reached.
    fn path_request(&self, kind: u32, arg1: u32, path: &[u8]) -> Option<u32> {
        let mut data = Vec::with_capacity(path.len() + 1);
        data.extend_from_slice(path);
        data.push(0);
        let mut msg = Message {
            kind,
            arg1,
            ..Message::default()
        };
        (ipc::send(self.pid, &mut msg, &data) >= 0).then_some(msg.arg1)
    }
}
