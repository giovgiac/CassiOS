/**
 * ata_client.hpp -- userspace ATA client helpers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_LIB_ATA_CLIENT_HPP_
#define USERSPACE_LIB_ATA_CLIENT_HPP_

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>

namespace cassio {

class AtaClient {
public:
    static inline bool readSector(u32 pid, u32 lba, u8* buf) {
        Message msg = {};
        msg.type = MessageType::AtaRead;
        msg.arg1 = lba;
        IPC::send(pid, &msg, buf, 512);
        return msg.arg1 == 0;
    }

    static inline bool writeSector(u32 pid, u32 lba, const u8* buf) {
        Message msg = {};
        msg.type = MessageType::AtaWrite;
        msg.arg1 = lba;
        IPC::send(pid, &msg, buf, 512);
        return msg.arg1 == 0;
    }
};

} // cassio

#endif // USERSPACE_LIB_ATA_CLIENT_HPP_
