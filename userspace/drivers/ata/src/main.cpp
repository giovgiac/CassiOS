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
#include <ata.hpp>

using namespace cassio;
using namespace cassio::ata;

extern "C" void _start() {
    Nameserver::registerName("ata");
    System::irqRegister(14);

    Ata drive;
    drive.init();

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
