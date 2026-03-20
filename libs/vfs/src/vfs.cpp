/**
 * vfs.cpp -- VFS service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/vfs.hpp>

using namespace std;

vfs::Vfs::Vfs() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("vfs");
    }
}

static u32 pathLen(const char* path) {
    u32 len = 0;
    while (path[len] != '\0')
        len++;
    return len;
}

u32 vfs::Vfs::mkdir(const char* path) {
    u32 len = pathLen(path);
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VfsMkdir;
    ipc::send(pid, &msg, path, len + 1);
    return msg.arg1;
}

u32 vfs::Vfs::remove(const char* path) {
    u32 len = pathLen(path);
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VfsRemove;
    ipc::send(pid, &msg, path, len + 1);
    return msg.arg1;
}

u32 vfs::Vfs::open(const char* path, bool create) {
    u32 len = pathLen(path);
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VfsOpen;
    msg.arg1 = create ? 1 : 0;
    ipc::send(pid, &msg, path, len + 1);
    return msg.arg1;
}

i32 vfs::Vfs::read(u32 handle, u32 offset, u8* buf, u32 bufLen) {
    for (u32 i = 0; i < bufLen; i++)
        buf[i] = 0;
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VfsRead;
    msg.arg1 = handle;
    msg.arg2 = offset;
    msg.arg3 = bufLen;
    ipc::send(pid, &msg, buf, bufLen);
    return static_cast<i32>(msg.arg1);
}

u32 vfs::Vfs::write(u32 handle, const u8* data, u32 len) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VfsWrite;
    msg.arg1 = handle;
    msg.arg2 = len;
    ipc::send(pid, &msg, data, len);
    return msg.arg1;
}

u32 vfs::Vfs::stat(const char* path) {
    u32 len = pathLen(path);
    ipc::Message msg = {};
    msg.type = ipc::MessageType::VfsStat;
    ipc::send(pid, &msg, path, len + 1);
    return msg.arg1;
}

bool vfs::Vfs::list(const char* path, u32 index, char* nameOut, u32 nameMax) {
    u32 pLen = pathLen(path);

    char buf[65];
    for (u32 k = 0; k < sizeof(buf); k++)
        buf[k] = 0;
    u32 i = 0;
    while (i < sizeof(buf) - 1 && i <= pLen) {
        buf[i] = path[i];
        i++;
    }

    ipc::Message msg = {};
    msg.type = ipc::MessageType::VfsList;
    msg.arg1 = index;
    msg.arg2 = pLen + 1;
    ipc::send(pid, &msg, buf, sizeof(buf));

    if (msg.arg1 == 0) {
        return false;
    }

    i = 0;
    u32 limit = nameMax - 1;
    while (i < limit && buf[i] != '\0') {
        nameOut[i] = buf[i];
        i++;
    }
    nameOut[i] = '\0';
    return true;
}
