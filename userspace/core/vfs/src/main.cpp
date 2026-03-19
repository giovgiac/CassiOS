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

#include <std/types.hpp>
#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/os.hpp>
#include <std/heap.hpp>
#include <std/str.hpp>
#include <fat32/filesystem.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::vfs;

static Fat32Filesystem fs;

extern "C" void _start() {
    ns::registerName("vfs");

    if (!fs.mount()) {
        // Mount failed -- hang.
        while (true) {
            ipc::Message msg;
            ipc::receive(&msg);
        }
    }

    // Data buffer for IPC messages -- large enough for file I/O.
    constexpr u32 BUF_SIZE = 4096;
    u8* dataBuf = static_cast<u8*>(heap::alloc(BUF_SIZE));

    while (true) {
        ipc::Message msg;
        i32 sender = ipc::receive(&msg, dataBuf, BUF_SIZE);

        ipc::Message reply = {};

        switch (msg.type) {
        case ipc::MessageType::VfsMkdir: {
            reply.arg1 = fs.createDirectory(reinterpret_cast<char*>(dataBuf)) ? 0 : 1;
            break;
        }

        case ipc::MessageType::VfsRemove: {
            reply.arg1 = fs.remove(reinterpret_cast<char*>(dataBuf)) ? 0 : 1;
            break;
        }

        case ipc::MessageType::VfsOpen: {
            bool create = (msg.arg1 != 0);
            reply.arg1 = fs.open(reinterpret_cast<char*>(dataBuf), create);
            break;
        }

        case ipc::MessageType::VfsRead: {
            u32 handle = msg.arg1;
            u32 offset = msg.arg2;
            u32 reqLen = msg.arg3;
            u32 maxRead = (reqLen < BUF_SIZE) ? reqLen : BUF_SIZE;

            i32 bytesRead = fs.read(handle, offset, dataBuf, maxRead);
            reply.arg1 = (bytesRead >= 0) ? static_cast<u32>(bytesRead) : 0;

            if (sender > 0) {
                u32 replyDataLen = (bytesRead > 0)
                                 ? static_cast<u32>(bytesRead) : 0;
                ipc::reply(static_cast<u32>(sender), &reply,
                           dataBuf, replyDataLen);
                continue;
            }
            break;
        }

        case ipc::MessageType::VfsWrite: {
            u32 handle = msg.arg1;
            u32 len = msg.arg2;
            reply.arg1 = fs.write(handle, dataBuf, len) ? 0 : 1;
            break;
        }

        case ipc::MessageType::VfsList: {
            u32 index = msg.arg1;
            char name[MAX_NAME];

            if (fs.listEntry(reinterpret_cast<char*>(dataBuf), index,
                             name, sizeof(name))) {
                reply.arg1 = 1;
                u32 nameLen = str::len(name);
                if (sender > 0) {
                    ipc::reply(static_cast<u32>(sender), &reply,
                               name, nameLen + 1);
                    continue;
                }
            } else {
                reply.arg1 = 0;
            }
            break;
        }

        case ipc::MessageType::VfsStat: {
            reply.arg1 = fs.stat(reinterpret_cast<char*>(dataBuf));
            break;
        }

        default:
            break;
        }

        if (sender > 0) {
            ipc::reply(static_cast<u32>(sender), &reply);
        }
    }
}
