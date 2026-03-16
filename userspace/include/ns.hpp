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

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>

namespace cassio {

class Nameserver {
public:
    static constexpr u32 PID = 1;

    static inline void packName(const char* name, Message& msg) {
        u32* words = &msg.arg1;
        for (u32 i = 0; i < 4; i++) {
            words[i] = 0;
        }
        for (u32 i = 0; name[i] && i < 16; i++) {
            words[i / 4] |= ((u32)(u8)name[i]) << ((i % 4) * 8);
        }
    }

    static inline void unpackName(const Message& msg, char* out) {
        const u32* words = &msg.arg1;
        for (u32 i = 0; i < 16; i++) {
            out[i] = (words[i / 4] >> ((i % 4) * 8)) & 0xFF;
        }
        out[16] = '\0';
    }

    static inline u32 registerName(const char* name) {
        Message msg = {};
        msg.type = MessageType::NsRegister;
        packName(name, msg);
        IPC::send(PID, &msg);
        return msg.arg1;
    }

    static inline u32 lookup(const char* name) {
        Message msg = {};
        msg.type = MessageType::NsLookup;
        packName(name, msg);
        IPC::send(PID, &msg);
        return msg.arg1;
    }
};

} // cassio

#endif // USERSPACE_LIB_NS_HPP_
