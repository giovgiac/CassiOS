// main.rs -- keyboard service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Userspace PS/2 keyboard driver. Receives IRQ 1 notifications via IPC,
// decodes scancodes into key codes, and serves them to clients via
// KbdRead, holding requests until a key is available.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

#[cfg(not(test))]
mod device;
mod keyboard;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::ipc::{self, Message, message_type};
    use cassio::{ns, os};

    use device::Device;
    use keyboard::Keyboard;

    const KBD_IRQ: u32 = 1;

    /// Send `key` to the reader `pid`; false if it is no longer waiting.
    fn reply_key(pid: u32, key: u8) -> bool {
        let mut reply = Message {
            arg1: key as u32,
            ..Message::default()
        };
        ipc::reply(pid, &mut reply, &[]) == 0
    }

    let device = Device::new();
    let mut keyboard = Keyboard::new();

    // Failures are logged and the service keeps running, so clients still
    // get replies (KbdRead simply waits for a key that never comes). The
    // name is registered only after activation: the mouse driver waits for
    // it, so the two never configure the shared controller at once.
    if os::irq_register(KBD_IRQ) < 0 {
        os::write(2, b"kbd: IRQ 1 is already registered\n");
    }
    if !device.activate() {
        os::write(2, b"kbd: PS/2 keyboard did not respond\n");
    }
    ns::register_name(b"kbd");

    loop {
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut []);

        match msg.kind {
            // IRQ notifications come from the kernel, with sender 0.
            message_type::IRQ_NOTIFY if sender == 0 => {
                device.drain(&mut keyboard);
                keyboard.serve(reply_key);
            }
            // Answered now if a key is buffered, otherwise when one arrives.
            message_type::KBD_READ if sender > 0 => keyboard.request(sender as u32, reply_key),
            _ if sender > 0 => {
                ipc::reply(sender as u32, &mut Message::default(), &[]);
            }
            _ => {}
        }
    }
}
