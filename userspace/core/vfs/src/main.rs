// main.rs -- VFS service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Serves the FAT32 volume on the ATA disk: paths, directories, and files
// opened by handle. Without a volume it still answers every request, with
// an error.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

mod cache;
mod dir;
mod fs;
mod layout;
mod service;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::ata::Ata;
    use cassio::ipc::{self, Message};
    use cassio::{ns, os};

    use dir::NAME_MAX;
    use fs::Filesystem;
    use service::{BUF_SIZE, ReplyData};

    // Registered first, as clients look the VFS up once and early; their
    // requests queue until the loop below starts receiving.
    ns::register_name(b"vfs");

    let mut fs = Filesystem::mount(Ata::connect());
    if fs.is_none() {
        os::write(2, b"vfs: no FAT32 volume on the disk\n");
    }
    // Heap buffers: the user stack is a single page.
    let mut buf = alloc::vec![0u8; BUF_SIZE];
    let mut name = alloc::boxed::Box::new([0u8; NAME_MAX]);

    loop {
        // Zeroed per request: receive does not report how many bytes
        // arrived, so a short path must not run into stale bytes.
        buf.fill(0);
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut buf);
        if sender <= 0 {
            continue;
        }
        let (mut reply, data) = service::handle(fs.as_mut(), &msg, &mut buf, &mut name);
        let data: &[u8] = match data {
            ReplyData::None => &[],
            ReplyData::Buffer(n) => &buf[..n],
            ReplyData::Name(n) => &name[..n],
        };
        ipc::reply(sender as u32, &mut reply, data);
    }
}
