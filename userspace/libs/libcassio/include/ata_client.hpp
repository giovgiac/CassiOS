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

#include <std/types.hpp>
#include <std/msg.hpp>
#include <ipc.hpp>

namespace cassio {

class AtaClient {
public:
    static inline bool readSector(std::u32 pid, std::u32 lba, std::u8* buf) {
        std::msg::Message msg = {};
        msg.type = std::msg::MessageType::AtaRead;
        msg.arg1 = lba;
        IPC::send(pid, &msg, buf, 512);
        return msg.arg1 == 0;
    }

    static inline bool writeSector(std::u32 pid, std::u32 lba, const std::u8* buf) {
        std::msg::Message msg = {};
        msg.type = std::msg::MessageType::AtaWrite;
        msg.arg1 = lba;
        IPC::send(pid, &msg, buf, 512);
        return msg.arg1 == 0;
    }
};

} // cassio

#endif // USERSPACE_LIB_ATA_CLIENT_HPP_
