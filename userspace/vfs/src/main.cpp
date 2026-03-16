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

static void unpackPath(const Message& msg, char* out, u32 maxLen) {
    const char* src = reinterpret_cast<const char*>(&msg.arg1);
    u32 limit = maxLen - 1;
    if (limit > 20) {
        limit = 20;
    }
    u32 i = 0;
    while (i < limit && src[i] != '\0') {
        out[i] = src[i];
        i++;
    }
    out[i] = '\0';
}

static void unpackPath16(const Message& msg, char* out, u32 maxLen) {
    const char* src = reinterpret_cast<const char*>(&msg.arg2);
    u32 limit = maxLen - 1;
    if (limit > 16) {
        limit = 16;
    }
    u32 i = 0;
    while (i < limit && src[i] != '\0') {
        out[i] = src[i];
        i++;
    }
    out[i] = '\0';
}

extern "C" void _start() {
    Nameserver::registerName("vfs");
    fs.init();
    initHandles();

    while (true) {
        Message msg;
        i32 sender = IPC::receive(&msg);

        Message reply = {};

        switch (msg.type) {
        case MessageType::VfsMkdir: {
            char path[MAX_NAME];
            unpackPath(msg, path, sizeof(path));
            u8 node = fs.createDirectory(path);
            reply.arg1 = (node != INVALID) ? 0 : 1;
            break;
        }

        case MessageType::VfsRemove: {
            char path[MAX_NAME];
            unpackPath(msg, path, sizeof(path));
            reply.arg1 = fs.remove(path) ? 0 : 1;
            break;
        }

        case MessageType::VfsOpen: {
            char path[MAX_NAME];
            unpackPath(msg, path, sizeof(path));

            u8 node = fs.resolve(path);
            if (node == INVALID) {
                node = fs.createFile(path);
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
            u8 node = getHandle(handle);

            u8* buf = reinterpret_cast<u8*>(&reply.arg2);
            i32 bytesRead = fs.read(node, offset, buf, 16);
            reply.arg1 = (bytesRead >= 0) ? static_cast<u32>(bytesRead) : 0;
            break;
        }

        case MessageType::VfsWrite: {
            u32 handle = msg.arg1;
            u32 len = msg.arg2;
            if (len > 12) {
                len = 12;
            }
            u8 node = getHandle(handle);
            const u8* data = reinterpret_cast<const u8*>(&msg.arg3);
            reply.arg1 = fs.write(node, data, len) ? 0 : 1;
            break;
        }

        case MessageType::VfsList: {
            u32 index = msg.arg1;
            char path[MAX_NAME];
            unpackPath16(msg, path, sizeof(path));

            u8 dir = fs.resolve(path);
            char name[MAX_NAME];
            NodeType type;

            if (fs.listEntry(dir, index, name, sizeof(name), type)) {
                reply.arg1 = 1;
                char* out = reinterpret_cast<char*>(&reply.arg2);
                u32 i = 0;
                while (i < 15 && name[i] != '\0') {
                    out[i] = name[i];
                    i++;
                }
                out[i] = '\0';
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
