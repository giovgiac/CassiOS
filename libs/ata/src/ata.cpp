/**
 * ata.cpp -- ATA service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ata.hpp>
#include <std/ipc.hpp>
#include <std/ns.hpp>

using namespace std;

ata::Ata::Ata() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("ata");
    }
}

bool ata::Ata::readSector(u32 lba, u8* buf) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::AtaRead;
    msg.arg1 = lba;
    ipc::send(pid, &msg, buf, SECTOR_SIZE);
    return msg.arg1 == 0;
}

bool ata::Ata::writeSector(u32 lba, const u8* buf) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::AtaWrite;
    msg.arg1 = lba;
    ipc::send(pid, &msg, buf, SECTOR_SIZE);
    return msg.arg1 == 0;
}
