// main.rs -- ATA service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Userspace ATA PIO driver for the primary master drive. Serves 512-byte
// sector reads and writes to clients (the VFS) via AtaRead and AtaWrite,
// with sector data carried in the IPC data buffer.

#![cfg_attr(not(test), no_std)]

mod drive;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::ipc::{self, Message, message_type};
    use cassio::{ns, os};

    use drive::{Drive, SECTOR_SIZE};

    const ATA_IRQ: u32 = 14;

    let mut drive = Drive::new();

    // Failures are logged and the service keeps running, answering every
    // request (with an error status if there is no drive). The name is
    // registered last, so no client waits on detection.
    if os::irq_register(ATA_IRQ) < 0 {
        os::write(2, b"ata: IRQ 14 is already registered\n");
    }
    if !drive.identify() {
        os::write(2, b"ata: no ATA drive on the primary master\n");
    }
    ns::register_name(b"ata");

    loop {
        // Zeroed per request: receive does not report how many bytes
        // arrived, so a short AtaWrite must not reuse stale data.
        let mut buf = [0u8; SECTOR_SIZE];
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut buf);

        // arg1 of the reply is 0 on success, 1 on failure.
        let mut reply = Message::default();
        match msg.kind {
            // IRQ notifications come from the kernel, with sender 0.
            message_type::IRQ_NOTIFY if sender == 0 => drive.ack_irq(),
            message_type::ATA_READ if sender > 0 => {
                let ok = drive.read_sector(msg.arg1, &mut buf);
                reply.arg1 = !ok as u32;
                ipc::reply(sender as u32, &mut reply, if ok { &buf } else { &[] });
            }
            message_type::ATA_WRITE if sender > 0 => {
                reply.arg1 = !drive.write_sector(msg.arg1, &buf) as u32;
                ipc::reply(sender as u32, &mut reply, &[]);
            }
            _ if sender > 0 => {
                ipc::reply(sender as u32, &mut reply, &[]);
            }
            _ => {}
        }
    }
}
