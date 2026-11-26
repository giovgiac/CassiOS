// main.rs -- nameserver service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// First userspace service (well-known PID 1). Provides name-to-PID
// lookup for service discovery via IPC.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

mod table;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use core::mem::size_of;
    use core::slice;

    use cassio::ipc::{self, Message, message_type};
    use cassio::ns::{self, Entry, MAX_NAME_LEN};

    use table::NsTable;

    const LIST_MAX: usize = 16;

    let mut table = NsTable::new();

    // Self-register (can't use IPC to send to ourselves).
    table.register(b"ns", ns::PID);

    loop {
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut []);
        if sender <= 0 {
            continue;
        }
        let sender = sender as u32;

        let mut reply = Message::default();
        let mut name = [0u8; MAX_NAME_LEN + 1];

        match msg.kind {
            message_type::NS_REGISTER => {
                reply.arg1 = table.register(ns::unpack_name(&msg, &mut name), sender);
                ipc::reply(sender, &mut reply, &[]);
            }
            message_type::NS_LOOKUP => {
                reply.arg1 = table.lookup(ns::unpack_name(&msg, &mut name));
                ipc::reply(sender, &mut reply, &[]);
            }
            message_type::NS_LIST_ALL => {
                let mut buf = [Entry::default(); LIST_MAX];
                let count = table.list_all(&mut buf);
                reply.arg1 = count as u32;
                let bytes = unsafe {
                    slice::from_raw_parts(buf.as_ptr() as *const u8, count * size_of::<Entry>())
                };
                ipc::reply(sender, &mut reply, bytes);
            }
            _ => {
                ipc::reply(sender, &mut reply, &[]);
            }
        }
    }
}
