/**
 * main.cpp -- nameserver service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * First userspace service (well-known PID 1). Provides name-to-PID
 * lookup for service discovery via IPC.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <system.hpp>
#include <userheap.hpp>
#include <table.hpp>

using namespace cassio;

static void* sbrkGrow(u32 size) {
    return System::sbrk(size);
}

static NsTable table;

extern "C" void _start() {
    UserHeap::init(sbrkGrow, 4096);

    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg);
        if (sender <= 0) {
            continue;
        }

        Message reply = {};
        char name[NsTable::MAX_NAME_LEN + 1];

        switch (msg.type) {
        case MessageType::NsRegister:
            Nameserver::unpackName(msg, name);
            reply.arg1 = table.registerName(name, static_cast<u32>(sender));
            break;
        case MessageType::NsLookup:
            Nameserver::unpackName(msg, name);
            reply.arg1 = table.lookup(name);
            break;
        }

        IPC::reply(static_cast<u32>(sender), &reply);
    }
}
