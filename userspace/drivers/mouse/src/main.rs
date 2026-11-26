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

#[cfg(not(test))]
mod device;
mod mouse;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::ipc::{self, Message, message_type};
    use cassio::{ns, os};

    use device::Device;
    use mouse::Mouse;

    const MOUSE_IRQ: u32 = 12;

    let device = Device::new();
    let mut mouse = Mouse::new();

    // Failures are logged and the service keeps answering MouseRead, so
    // clients never block on a missing or unresponsive mouse.
    ns::register_name(b"mouse");
    if os::irq_register(MOUSE_IRQ) < 0 {
        os::write(2, b"mouse: IRQ 12 is already registered\n");
    }
    if !device.activate() {
        os::write(2, b"mouse: PS/2 mouse did not respond\n");
    }

    loop {
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut []);

        match msg.kind {
            // IRQ notifications come from the kernel, with sender 0.
            message_type::IRQ_NOTIFY if sender == 0 => device.drain(&mut mouse),
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
