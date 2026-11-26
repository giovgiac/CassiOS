// main.rs -- keyboard service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Userspace PS/2 keyboard driver. Receives IRQ 1 notifications via IPC,
// decodes scancodes into key codes, and serves them to clients via
// KbdRead, holding a request until a key is available.

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

    fn reply_key(pid: u32, key: u8) {
        let mut reply = Message {
            arg1: key as u32,
            ..Message::default()
        };
        ipc::reply(pid, &mut reply, &[]);
    }

    let device = Device::new();
    let mut keyboard = Keyboard::new();

    // A KbdRead that arrived while no key was buffered; answered when one
    // arrives.
    let mut pending_reader: Option<u32> = None;

    // Failures are logged and the service keeps running, so clients still
    // get replies (KbdRead simply waits for a key that never comes).
    ns::register_name(b"kbd");
    if os::irq_register(KBD_IRQ) < 0 {
        os::write(2, b"kbd: IRQ 1 is already registered\n");
    }
    if !device.activate() {
        os::write(2, b"kbd: PS/2 keyboard did not respond\n");
    }

    loop {
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut []);

        match msg.kind {
            // IRQ notifications come from the kernel, with sender 0.
            message_type::IRQ_NOTIFY if sender == 0 => {
                device.drain(&mut keyboard);
                if let Some(pid) = pending_reader {
                    if let Some(key) = keyboard.read() {
                        reply_key(pid, key);
                        pending_reader = None;
                    }
                }
            }
            message_type::KBD_READ if sender > 0 => match keyboard.read() {
                Some(key) => reply_key(sender as u32, key),
                None => pending_reader = Some(sender as u32),
            },
            _ if sender > 0 => {
                ipc::reply(sender as u32, &mut Message::default(), &[]);
            }
            _ => {}
        }
    }
}
