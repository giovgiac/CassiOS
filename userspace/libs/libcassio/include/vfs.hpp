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
    static inline u32 mkdir(u32 pid, const char* path) {
        u32 len = 0;
        while (path[len] != '\0') len++;
        Message msg = {};
        msg.type = MessageType::VfsMkdir;
        IPC::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline u32 remove(u32 pid, const char* path) {
        u32 len = 0;
        while (path[len] != '\0') len++;
        Message msg = {};
        msg.type = MessageType::VfsRemove;
        IPC::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline u32 open(u32 pid, const char* path, bool create = false) {
        u32 len = 0;
        while (path[len] != '\0') len++;
        Message msg = {};
        msg.type = MessageType::VfsOpen;
        msg.arg1 = create ? 1 : 0;
        IPC::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline i32 read(u32 pid, u32 handle, u32 offset, u8* buf,
                           u32 bufLen) {
        Message msg = {};
        msg.type = MessageType::VfsRead;
        msg.arg1 = handle;
        msg.arg2 = offset;
        msg.arg3 = bufLen;
        IPC::send(pid, &msg, buf, bufLen);
        return static_cast<i32>(msg.arg1);
    }

    static inline u32 write(u32 pid, u32 handle, const u8* data, u32 len) {
        Message msg = {};
        msg.type = MessageType::VfsWrite;
        msg.arg1 = handle;
        msg.arg2 = len;
        IPC::send(pid, &msg, data, len);
        return msg.arg1;
    }

    // Returns 0 = not found, 1 = file, 2 = directory.
    static inline u32 stat(u32 pid, const char* path) {
        u32 len = 0;
        while (path[len] != '\0') len++;
        Message msg = {};
        msg.type = MessageType::VfsStat;
        IPC::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline bool list(u32 pid, const char* path, u32 index,
                            char* nameOut, u32 nameMax) {
        u32 pathLen = 0;
        while (path[pathLen] != '\0') pathLen++;

        // Use a local buffer: send the path, reply overwrites with the name.
        char buf[64];
        u32 i = 0;
        while (i < sizeof(buf) - 1 && i <= pathLen) {
            buf[i] = path[i];
            i++;
        }

        Message msg = {};
        msg.type = MessageType::VfsList;
        msg.arg1 = index;
        msg.arg2 = pathLen + 1;
        IPC::send(pid, &msg, buf, sizeof(buf));

        if (msg.arg1 == 0) {
            return false;
        }

        // buf now contains the reply name.
        i = 0;
        u32 limit = nameMax - 1;
        while (i < limit && buf[i] != '\0') {
            nameOut[i] = buf[i];
            i++;
        }
        nameOut[i] = '\0';
        return true;
    }
};

} // cassio

#endif // USERSPACE_LIB_VFS_HPP_
