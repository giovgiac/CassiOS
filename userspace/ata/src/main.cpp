/**
 * main.cpp -- ATA service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace ATA PIO driver. Receives IRQ 14 notifications via IPC,
 * reads/writes sectors via port I/O, and serves data to clients via
 * MSG_ATA_READ and MSG_ATA_WRITE. Bulk sector transfers are limited
 * to register-sized payloads (~16 bytes) until buffer IPC is added.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <system.hpp>
#include <vga.hpp>
#include <ata.hpp>

using namespace cassio;
using namespace cassio::ata;

static void vga_put_dec(u32 vgaPid, u32 value) {
    if (value == 0) {
        Vga::write(vgaPid, "0");
        return;
    }
    char buf[12];
    i32 i = 0;
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }
    for (i32 j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    buf[i] = '\0';
    Vga::write(vgaPid, buf);
}

extern "C" void _start() {
    Nameserver::registerName("ata");
    System::irqRegister(14);

    Ata drive;
    drive.init();

    // Print drive info to VGA terminal.
    u32 vgaPid = Nameserver::lookup("vga");
    if (vgaPid != 0) {
        if (drive.isPresent()) {
            Vga::write(vgaPid, "ATA: ");
            Vga::write(vgaPid, drive.getModel());
            Vga::write(vgaPid, " (");
            vga_put_dec(vgaPid, drive.getSectors());
            Vga::write(vgaPid, " sectors, ");
            vga_put_dec(vgaPid, drive.getSectors() / 2);
            Vga::write(vgaPid, " KiB)\n");
        } else {
            Vga::write(vgaPid, "ATA: no drive detected\n");
        }
    }

    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg);

        switch (msg.type) {
        case MessageType::IrqNotify:
            drive.handleIrq();
            break;

        case MessageType::AtaRead: {
            Message reply = {};
            u32 lba = msg.arg1;
            u32 offset = msg.arg2;

            u8* buf = reinterpret_cast<u8*>(&reply.arg2);
            i32 n = drive.read(lba, offset, buf, 16);
            reply.arg1 = (n >= 0) ? static_cast<u32>(n) : 0;

            if (sender > 0) {
                IPC::reply(static_cast<u32>(sender), &reply);
            }
            break;
        }

        case MessageType::AtaWrite: {
            Message reply = {};
            u32 lba = msg.arg1;
            u32 offset = msg.arg2;
            u32 len = msg.arg3;
            if (len > 8) len = 8;

            const u8* buf = reinterpret_cast<const u8*>(&msg.arg4);
            reply.arg1 = drive.write(lba, offset, buf, len) ? 0 : 1;

            if (sender > 0) {
                IPC::reply(static_cast<u32>(sender), &reply);
            }
            break;
        }

        default:
            if (sender > 0) {
                Message reply = {};
                IPC::reply(static_cast<u32>(sender), &reply);
            }
            break;
        }
    }
}
