/**
 * main.cpp -- nameserver service
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * First userspace service (well-known PID 1). Provides name-to-PID
 * lookup for service discovery. Handles NsRegister and NsLookup
 * messages in a receive loop.
 *
 */

#include <types.hpp>
#include <string.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <nameserver.hpp>

using namespace cassio;

struct NsEntry {
    char name[17];
    u32 pid;
};

static NsEntry table[16];
static u32 count = 0;

static u32 lookup(const char* name) {
    for (u32 i = 0; i < count; i++) {
        if (streq(table[i].name, name)) {
            return table[i].pid;
        }
    }
    return 0;
}

static u32 registerName(const char* name, u32 pid) {
    if (count >= 16 || lookup(name) != 0) {
        return 0;
    }
    strcpy(table[count].name, name, 17);
    table[count].pid = pid;
    count++;
    return 1;
}

extern "C" void _start() {
    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg);
        if (sender <= 0) {
            continue;
        }

        Message reply = {};
        char name[17];

        switch (msg.type) {
        case MessageType::NsRegister:
            Nameserver::unpackName(msg, name);
            reply.arg1 = registerName(name, static_cast<u32>(sender));
            break;
        case MessageType::NsLookup:
            Nameserver::unpackName(msg, name);
            reply.arg1 = lookup(name);
            break;
        }

        IPC::reply(static_cast<u32>(sender), &reply);
    }
}
