/**
 * vfs.hpp -- userspace VFS client helpers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_LIB_VFS_HPP_
#define USERSPACE_LIB_VFS_HPP_

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>

namespace cassio {

class Vfs {
public:
    static inline void packPath(Message& msg, const char* path) {
        char* dst = reinterpret_cast<char*>(&msg.arg1);
        u32 i = 0;
        while (i < 20 && path[i] != '\0') {
            dst[i] = path[i];
            i++;
        }
        if (i < 20) {
            dst[i] = '\0';
        }
    }

    static inline void packPath16(Message& msg, const char* path) {
        char* dst = reinterpret_cast<char*>(&msg.arg2);
        u32 i = 0;
        while (i < 16 && path[i] != '\0') {
            dst[i] = path[i];
            i++;
        }
        if (i < 16) {
            dst[i] = '\0';
        }
    }

    static inline u32 mkdir(u32 pid, const char* path) {
        Message msg = {};
        msg.type = MessageType::VfsMkdir;
        packPath(msg, path);
        IPC::send(pid, &msg);
        return msg.arg1;
    }

    static inline u32 remove(u32 pid, const char* path) {
        Message msg = {};
        msg.type = MessageType::VfsRemove;
        packPath(msg, path);
        IPC::send(pid, &msg);
        return msg.arg1;
    }

    static inline u32 open(u32 pid, const char* path) {
        Message msg = {};
        msg.type = MessageType::VfsOpen;
        packPath(msg, path);
        IPC::send(pid, &msg);
        return msg.arg1;
    }

    static inline i32 read(u32 pid, u32 handle, u32 offset, u8* buf,
                           u32 bufLen) {
        Message msg = {};
        msg.type = MessageType::VfsRead;
        msg.arg1 = handle;
        msg.arg2 = offset;
        IPC::send(pid, &msg);

        u32 bytesRead = msg.arg1;
        const u8* src = reinterpret_cast<const u8*>(&msg.arg2);
        u32 toCopy = bytesRead < bufLen ? bytesRead : bufLen;
        for (u32 i = 0; i < toCopy; i++) {
            buf[i] = src[i];
        }
        return static_cast<i32>(bytesRead);
    }

    static inline u32 write(u32 pid, u32 handle, const u8* data, u32 len) {
        Message msg = {};
        msg.type = MessageType::VfsWrite;
        msg.arg1 = handle;
        msg.arg2 = len;
        u8* dst = reinterpret_cast<u8*>(&msg.arg3);
        u32 limit = len < 12 ? len : 12;
        for (u32 i = 0; i < limit; i++) {
            dst[i] = data[i];
        }
        IPC::send(pid, &msg);
        return msg.arg1;
    }

    static inline bool list(u32 pid, const char* path, u32 index,
                            char* nameOut, u32 nameMax) {
        Message msg = {};
        msg.type = MessageType::VfsList;
        msg.arg1 = index;
        packPath16(msg, path);
        IPC::send(pid, &msg);

        if (msg.arg1 == 0) {
            return false;
        }
        const char* src = reinterpret_cast<const char*>(&msg.arg2);
        u32 i = 0;
        u32 limit = nameMax - 1;
        if (limit > 16) {
            limit = 16;
        }
        while (i < limit && src[i] != '\0') {
            nameOut[i] = src[i];
            i++;
        }
        nameOut[i] = '\0';
        return true;
    }
};

} // cassio

#endif // USERSPACE_LIB_VFS_HPP_
