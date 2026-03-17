/**
 * main.cpp -- ATA service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace ATA PIO driver. Receives IRQ 14 notifications via IPC,
 * reads/writes sectors via port I/O, and serves full 512-byte sector
 * transfers to clients via AtaRead and AtaWrite using IPC data buffers.
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

    u8 sectorBuf[SECTOR_SIZE];

    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg, sectorBuf, SECTOR_SIZE);

        switch (msg.type) {
        case MessageType::IrqNotify:
            drive.handleIrq();
            break;

        case MessageType::AtaRead: {
            Message reply = {};
            u32 lba = msg.arg1;
            reply.arg1 = drive.readSector(lba, sectorBuf) ? 0 : 1;

            if (sender > 0) {
                IPC::reply(static_cast<u32>(sender), &reply, sectorBuf,
                           SECTOR_SIZE);
            }
            break;
        }

        case MessageType::AtaWrite: {
            Message reply = {};
            u32 lba = msg.arg1;
            reply.arg1 = drive.writeSector(lba, sectorBuf) ? 0 : 1;

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
