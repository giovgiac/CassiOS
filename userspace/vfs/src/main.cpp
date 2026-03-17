/**
 * main.cpp -- VFS service entry point
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Userspace in-memory filesystem service. Registers as "vfs" with
 * the nameserver and handles filesystem requests via IPC.
 *
 */

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>
#include <filesystem.hpp>

using namespace cassio;
using namespace cassio::vfs;

static Filesystem fs;

static constexpr u32 MAX_HANDLES = 16;
static u8 handles[MAX_HANDLES];

static void initHandles() {
    for (u32 i = 0; i < MAX_HANDLES; i++) {
        handles[i] = INVALID;
    }
}

static u32 openHandle(u8 nodeIndex) {
    for (u32 i = 0; i < MAX_HANDLES; i++) {
        if (handles[i] == INVALID) {
            handles[i] = nodeIndex;
            return i + 1;
        }
    }
    return 0;
}

static u8 getHandle(u32 handle) {
    if (handle == 0 || handle > MAX_HANDLES) {
        return INVALID;
    }
    return handles[handle - 1];
}

extern "C" void _start() {
    Nameserver::registerName("vfs");
    fs.init();
    initHandles();

    while (true) {
        Message msg;
        char dataBuf[MAX_FILE_DATA];
        i32 sender = IPC::receive(&msg, dataBuf, sizeof(dataBuf));

        Message reply = {};

        switch (msg.type) {
        case MessageType::VfsMkdir: {
            // Path is in dataBuf (null-terminated).
            u8 node = fs.createDirectory(dataBuf);
            reply.arg1 = (node != INVALID) ? 0 : 1;
            break;
        }

        case MessageType::VfsRemove: {
            reply.arg1 = fs.remove(dataBuf) ? 0 : 1;
            break;
        }

        case MessageType::VfsOpen: {
            u8 node = fs.resolve(dataBuf);
            if (node == INVALID) {
                node = fs.createFile(dataBuf);
            }

            if (node != INVALID && fs.isFile(node)) {
                reply.arg1 = openHandle(node);
            } else {
                reply.arg1 = 0;
            }
            break;
        }

        case MessageType::VfsRead: {
            u32 handle = msg.arg1;
            u32 offset = msg.arg2;
            u32 reqLen = msg.arg3;
            u8 node = getHandle(handle);

            u8 readBuf[MAX_FILE_DATA];
            u32 maxRead = reqLen < MAX_FILE_DATA ? reqLen : MAX_FILE_DATA;
            i32 bytesRead = fs.read(node, offset, readBuf, maxRead);
            reply.arg1 = (bytesRead >= 0) ? static_cast<u32>(bytesRead) : 0;

            if (sender > 0) {
                u32 replyDataLen = bytesRead > 0
                                 ? static_cast<u32>(bytesRead) : 0;
                IPC::reply(static_cast<u32>(sender), &reply,
                           readBuf, replyDataLen);
                continue;
            }
            break;
        }

        case MessageType::VfsWrite: {
            u32 handle = msg.arg1;
            u32 len = msg.arg2;
            u8 node = getHandle(handle);
            reply.arg1 = fs.write(node, (const u8*)dataBuf, len) ? 0 : 1;
            break;
        }

        case MessageType::VfsList: {
            u32 index = msg.arg1;
            // dataBuf contains the path (null-terminated).

            u8 dir = fs.resolve(dataBuf);
            char name[MAX_NAME];
            NodeType type;

            if (fs.listEntry(dir, index, name, sizeof(name), type)) {
                reply.arg1 = 1;
                u32 nameLen = 0;
                while (name[nameLen] != '\0') nameLen++;
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
