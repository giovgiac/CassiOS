// main.rs -- mouse service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Userspace PS/2 mouse driver. Receives IRQ 12 notifications via IPC,
// reads mouse data from the PS/2 controller, and serves mouse state to
// clients via MouseRead.

#![cfg_attr(not(test), no_std)]

mod mouse;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::io::{Port, PortType};
    use cassio::ipc::{self, Message, message_type};
    use cassio::{ns, os};

    use mouse::Mouse;

    const MOUSE_IRQ: u32 = 12;

    let cmd = Port::<u8>::new(PortType::KbdCommand);
    let data = Port::<u8>::new(PortType::KbdData);
    let mut mouse = Mouse::new();

    ns::register_name(b"mouse");
    os::irq_register(MOUSE_IRQ);

    // Enable the mouse on the PS/2 controller.
    cmd.write(0xA8);

    // Read and modify the command byte: enable the mouse IRQ (bit 1).
    cmd.write(0x20);
    let status = data.read() | 0x02;
    cmd.write(0x60);
    data.write(status);

    // Send 0xF4 (enable reporting) to the mouse and drain its ACK.
    cmd.write(0xD4);
    data.write(0xF4);
    data.read();

    loop {
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut []);

        match msg.kind {
            message_type::IRQ_NOTIFY => {
                // Read all pending mouse bytes.
                while cmd.read() & 0x21 != 0 {
                    mouse.handle_byte(data.read());
                }
            }
            message_type::MOUSE_READ if sender > 0 => {
                let (buttons, dx, dy) = mouse.read_state();
                let mut reply = Message {
                    arg1: buttons as u32,
                    arg2: dx as u32,
                    arg3: dy as u32,
                    ..Message::default()
                };
                ipc::reply(sender as u32, &mut reply, &[]);
            }
            _ if sender > 0 => {
                ipc::reply(sender as u32, &mut Message::default(), &[]);
            }
            _ => {}
        }
    }
}
