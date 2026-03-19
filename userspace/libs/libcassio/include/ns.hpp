/**
 * ns.hpp -- userspace nameserver client helpers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_LIB_NS_HPP_
#define USERSPACE_LIB_NS_HPP_

#include <std/types.hpp>
#include <std/ipc.hpp>

namespace cassio {

struct NsEntry {
    char name[20];  // MAX_NAME_LEN + 1 + padding for std::u32 alignment.
    std::u32 pid;
};

class Nameserver {
public:
    static constexpr std::u32 PID = 1;

    static inline void packName(const char* name, std::ipc::Message& msg) {
        std::u32* words = &msg.arg1;
        for (std::u32 i = 0; i < 4; i++) {
            words[i] = 0;
        }
        for (std::u32 i = 0; name[i] && i < 16; i++) {
            words[i / 4] |= ((std::u32)(std::u8)name[i]) << ((i % 4) * 8);
        }
    }

    static inline void unpackName(const std::ipc::Message& msg, char* out) {
        const std::u32* words = &msg.arg1;
        for (std::u32 i = 0; i < 16; i++) {
            out[i] = (words[i / 4] >> ((i % 4) * 8)) & 0xFF;
        }
        out[16] = '\0';
    }

    static inline std::u32 registerName(const char* name) {
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::NsRegister;
        packName(name, msg);
        std::ipc::send(PID, &msg);
        return msg.arg1;
    }

    static inline std::u32 lookup(const char* name) {
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::NsLookup;
        packName(name, msg);
        std::ipc::send(PID, &msg);
        return msg.arg1;
    }

    static inline std::u32 listAll(NsEntry* buf, std::u32 maxEntries) {
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::NsListAll;
        std::ipc::send(PID, &msg, buf, maxEntries * sizeof(NsEntry));
        return msg.arg1;
    }
};

} // cassio

#endif // USERSPACE_LIB_NS_HPP_
