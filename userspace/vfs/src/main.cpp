/**
 * main.cpp -- VFS service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace FAT32 filesystem service. Registers as "vfs" with
 * the nameserver and handles filesystem requests via IPC.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <system.hpp>
#include <userheap.hpp>
#include <string.hpp>
#include <fat32.hpp>

using namespace cassio;
using namespace cassio::vfs;

static void* sbrkGrow(u32 size) {
    return System::sbrk(size);
}

static Fat32 fs;

extern "C" void _start() {
    UserHeap::init(sbrkGrow, 4096);
    Nameserver::registerName("vfs");

    u32 ataPid = Nameserver::lookup("ata");
    if (!fs.mount(ataPid)) {
        // Mount failed -- hang.
        while (true) {
            Message msg;
            IPC::receive(&msg);
        }
    }

    // Data buffer for IPC messages -- large enough for file I/O.
    constexpr u32 BUF_SIZE = 4096;
    u8* dataBuf = static_cast<u8*>(UserHeap::alloc(BUF_SIZE));

    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg, dataBuf, BUF_SIZE);

        Message reply = {};

        switch (msg.type) {
        case MessageType::VfsMkdir: {
            reply.arg1 = fs.createDirectory(reinterpret_cast<char*>(dataBuf)) ? 0 : 1;
            break;
        }

        case MessageType::VfsRemove: {
            reply.arg1 = fs.remove(reinterpret_cast<char*>(dataBuf)) ? 0 : 1;
            break;
        }

        case MessageType::VfsOpen: {
            reply.arg1 = fs.open(reinterpret_cast<char*>(dataBuf));
            break;
        }

        case MessageType::VfsRead: {
            u32 handle = msg.arg1;
            u32 offset = msg.arg2;
            u32 reqLen = msg.arg3;
            u32 maxRead = (reqLen < BUF_SIZE) ? reqLen : BUF_SIZE;

            i32 bytesRead = fs.read(handle, offset, dataBuf, maxRead);
            reply.arg1 = (bytesRead >= 0) ? static_cast<u32>(bytesRead) : 0;

            if (sender > 0) {
                u32 replyDataLen = (bytesRead > 0)
                                 ? static_cast<u32>(bytesRead) : 0;
                IPC::reply(static_cast<u32>(sender), &reply,
                           dataBuf, replyDataLen);
                continue;
            }
            break;
        }

        case MessageType::VfsWrite: {
            u32 handle = msg.arg1;
            u32 len = msg.arg2;
            reply.arg1 = fs.write(handle, dataBuf, len) ? 0 : 1;
            break;
        }

        case MessageType::VfsList: {
            u32 index = msg.arg1;
            char name[MAX_NAME];

            if (fs.listEntry(reinterpret_cast<char*>(dataBuf), index,
                             name, sizeof(name))) {
                reply.arg1 = 1;
                u32 nameLen = strlen(name);
                if (sender > 0) {
                    IPC::reply(static_cast<u32>(sender), &reply,
                               name, nameLen + 1);
                    continue;
                }
            } else {
                reply.arg1 = 0;
            }
            break;
        }

        default:
            break;
        }

        if (sender > 0) {
            IPC::reply(static_cast<u32>(sender), &reply);
        }
    }
}
