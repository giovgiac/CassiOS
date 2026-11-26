// service.rs -- VFS request handling
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Paths arrive NUL-terminated in the request's data buffer. Without a
// mounted volume every request is answered with its error value, so
// clients never wait forever.

use cassio::ipc::{Message, message_type};

use crate::cache::BlockDevice;
use crate::dir::NAME_MAX;
use crate::fs::Filesystem;

/// Size of the request/reply data buffer: the largest read or write.
pub const BUF_SIZE: usize = 4096;

/// Which data goes back with the reply.
#[derive(Debug, PartialEq, Eq)]
pub enum ReplyData {
    None,
    /// The first n bytes of the request buffer.
    Buffer(usize),
    /// The first n bytes of the name buffer.
    Name(usize),
}

/// The text of a NUL-terminated path at the start of `buf`.
fn path(buf: &[u8]) -> &[u8] {
    let end = buf.iter().position(|&c| c == 0).unwrap_or(buf.len());
    &buf[..end]
}

/// Answer one request. `buf` holds the request data and receives read
/// data; `name` receives listed names and their NUL.
pub fn handle<D: BlockDevice>(
    fs: Option<&mut Filesystem<D>>,
    msg: &Message,
    buf: &mut [u8],
    name: &mut [u8; NAME_MAX + 1],
) -> (Message, ReplyData) {
    // 0 means success for mkdir, remove, write and close.
    let status = |ok: bool| !ok as u32;
    let mut reply = Message::default();
    let mut data = ReplyData::None;

    let Some(fs) = fs else {
        reply.arg1 = match msg.kind {
            message_type::VFS_MKDIR
            | message_type::VFS_REMOVE
            | message_type::VFS_WRITE
            | message_type::VFS_CLOSE => 1,
            _ => 0,
        };
        return (reply, data);
    };

    match msg.kind {
        message_type::VFS_MKDIR => reply.arg1 = status(fs.mkdir(path(buf))),
        message_type::VFS_REMOVE => reply.arg1 = status(fs.remove(path(buf))),
        message_type::VFS_OPEN => reply.arg1 = fs.open(path(buf), msg.arg1 != 0),
        message_type::VFS_CLOSE => reply.arg1 = status(fs.close(msg.arg1)),
        message_type::VFS_STAT => reply.arg1 = fs.stat(path(buf)) as u32,
        message_type::VFS_READ => {
            let len = (msg.arg3 as usize).min(buf.len());
            let read = fs.read(msg.arg1, msg.arg2, &mut buf[..len]).unwrap_or(0);
            reply.arg1 = read as u32;
            data = ReplyData::Buffer(read);
        }
        message_type::VFS_WRITE => {
            let len = msg.arg2 as usize;
            reply.arg1 = status(len <= buf.len() && fs.write(msg.arg1, &buf[..len]));
        }
        message_type::VFS_LIST => {
            if let Some(len) = fs.list(path(buf), msg.arg1 as usize, &mut name[..NAME_MAX]) {
                // The client expects the name NUL-terminated.
                name[len] = 0;
                reply.arg1 = 1;
                data = ReplyData::Name(len + 1);
            }
        }
        _ => {}
    }
    (reply, data)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cache::tests::MemDisk;
    use crate::fs::Stat;

    struct Client {
        fs: Option<Filesystem<MemDisk>>,
        buf: alloc::vec::Vec<u8>,
        name: [u8; NAME_MAX + 1],
    }

    impl Client {
        fn new(fs: Option<Filesystem<MemDisk>>) -> Self {
            Self {
                fs,
                buf: alloc::vec![0; BUF_SIZE],
                name: [0; NAME_MAX + 1],
            }
        }

        fn request(
            &mut self,
            kind: u32,
            args: [u32; 3],
            data: &[u8],
        ) -> (u32, alloc::vec::Vec<u8>) {
            self.buf.fill(0);
            self.buf[..data.len()].copy_from_slice(data);
            let msg = Message {
                kind,
                arg1: args[0],
                arg2: args[1],
                arg3: args[2],
                ..Message::default()
            };
            let (reply, out) = handle(self.fs.as_mut(), &msg, &mut self.buf, &mut self.name);
            let bytes = match out {
                ReplyData::None => alloc::vec::Vec::new(),
                ReplyData::Buffer(n) => self.buf[..n].to_vec(),
                ReplyData::Name(n) => self.name[..n].to_vec(),
            };
            (reply.arg1, bytes)
        }
    }

    fn mounted() -> Client {
        // Reuse the filesystem tests' formatter through a fresh volume.
        let disk = {
            let mut disk = MemDisk::new(8192);
            let fat_size = (8192 - 32 + 2u32).div_ceil(128);
            disk.sectors[0] = crate::layout::tests::boot_sector(8192, 1, fat_size);
            for copy in 0..2 {
                let fat = &mut disk.sectors[(32 + copy * fat_size) as usize];
                fat[..12].copy_from_slice(&[
                    0xF8, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0x0F, 0xF8, 0xFF, 0xFF, 0x0F,
                ]);
            }
            disk
        };
        Client::new(Some(Filesystem::mount(disk).unwrap()))
    }

    #[test]
    fn protocol_round_trip() {
        let mut c = mounted();
        assert_eq!(c.request(message_type::VFS_MKDIR, [0; 3], b"/d\0").0, 0);
        let (h, _) = c.request(message_type::VFS_OPEN, [1, 0, 0], b"/d/f.txt\0");
        assert_ne!(h, 0);
        assert_eq!(c.request(message_type::VFS_WRITE, [h, 5, 0], b"hello").0, 0);
        assert_eq!(
            c.request(message_type::VFS_READ, [h, 1, 100], b""),
            (4, b"ello".to_vec())
        );
        assert_eq!(
            c.request(message_type::VFS_STAT, [0; 3], b"/d\0").0,
            Stat::Directory as u32
        );
        assert_eq!(
            c.request(message_type::VFS_STAT, [0; 3], b"/d/f.txt\0").0,
            Stat::File as u32
        );
        assert_eq!(
            c.request(message_type::VFS_LIST, [0, 3, 0], b"/d\0"),
            (1, b"f.txt\0".to_vec())
        );
        assert_eq!(c.request(message_type::VFS_LIST, [1, 3, 0], b"/d\0").0, 0);
        assert_eq!(c.request(message_type::VFS_CLOSE, [h, 0, 0], b"").0, 0);
        assert_eq!(c.request(message_type::VFS_CLOSE, [h, 0, 0], b"").0, 1);
        assert_eq!(
            c.request(message_type::VFS_REMOVE, [0; 3], b"/d/f.txt\0").0,
            0
        );
        assert_eq!(c.request(message_type::VFS_REMOVE, [0; 3], b"/d\0").0, 0);
    }

    #[test]
    fn longest_names_are_listed_with_their_nul() {
        let mut c = mounted();
        let mut path = b"/".to_vec();
        path.extend_from_slice(&[b'n'; NAME_MAX]);
        path.push(0);
        assert_ne!(c.request(message_type::VFS_OPEN, [1, 0, 0], &path).0, 0);
        let (found, name) = c.request(message_type::VFS_LIST, [0, 0, 0], b"/\0");
        assert_eq!((found, name.len()), (1, NAME_MAX + 1));
        assert_eq!(&name[..NAME_MAX], &path[1..=NAME_MAX]);
        assert_eq!(name[NAME_MAX], 0);
    }

    #[test]
    fn oversized_write_is_rejected() {
        let mut c = mounted();
        let (h, _) = c.request(message_type::VFS_OPEN, [1, 0, 0], b"/f\0");
        assert_eq!(
            c.request(message_type::VFS_WRITE, [h, BUF_SIZE as u32 + 1, 0], b"")
                .0,
            1
        );
    }

    #[test]
    fn read_is_capped_by_the_buffer() {
        let mut c = mounted();
        let (h, _) = c.request(message_type::VFS_OPEN, [1, 0, 0], b"/f\0");
        let data = alloc::vec![7u8; BUF_SIZE];
        assert_eq!(
            c.request(message_type::VFS_WRITE, [h, BUF_SIZE as u32, 0], &data)
                .0,
            0
        );
        let (n, out) = c.request(message_type::VFS_READ, [h, 0, u32::MAX], b"");
        assert_eq!((n as usize, out.len()), (BUF_SIZE, BUF_SIZE));
    }

    #[test]
    fn unmounted_answers_every_request_with_an_error() {
        let mut c = Client::new(None);
        let expected = [
            (message_type::VFS_MKDIR, 1),
            (message_type::VFS_REMOVE, 1),
            (message_type::VFS_OPEN, 0),
            (message_type::VFS_READ, 0),
            (message_type::VFS_WRITE, 1),
            (message_type::VFS_LIST, 0),
            (message_type::VFS_STAT, 0),
            (message_type::VFS_CLOSE, 1),
        ];
        for (kind, arg1) in expected {
            assert_eq!(
                c.request(kind, [1, 1, 1], b"/x\0"),
                (arg1, alloc::vec::Vec::new())
            );
        }
    }
}
