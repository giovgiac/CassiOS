/**
 * ns.cpp -- nameserver client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ns.hpp>

using namespace std;

void ns::packName(const char* name, ipc::Message& msg) {
    u32* words = &msg.arg1;
    for (u32 i = 0; i < 4; i++) {
        words[i] = 0;
    }
    for (u32 i = 0; name[i] && i < MAX_NAME_LEN; i++) {
        words[i / 4] |= (static_cast<u32>(static_cast<u8>(name[i]))) << ((i % 4) * 8);
    }
}

void ns::unpackName(const ipc::Message& msg, char* out) {
    const u32* words = &msg.arg1;
    for (u32 i = 0; i < MAX_NAME_LEN; i++) {
        out[i] = (words[i / 4] >> ((i % 4) * 8)) & 0xFF;
    }
    out[MAX_NAME_LEN] = '\0';
}

u32 ns::registerName(const char* name) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::NsRegister;
    packName(name, msg);
    ipc::send(PID, &msg);
    return msg.arg1;
}

u32 ns::lookup(const char* name) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::NsLookup;
    packName(name, msg);
    ipc::send(PID, &msg);
    return msg.arg1;
}

u32 ns::listAll(Entry* buf, u32 maxEntries) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::NsListAll;
    ipc::send(PID, &msg, buf, maxEntries * sizeof(Entry));
    return msg.arg1;
}
