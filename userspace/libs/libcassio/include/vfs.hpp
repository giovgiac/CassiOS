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

#include <std/types.hpp>
#include <std/ipc.hpp>

namespace cassio {

class Vfs {
public:
    static inline std::u32 mkdir(std::u32 pid, const char* path) {
        std::u32 len = 0;
        while (path[len] != '\0') len++;
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::VfsMkdir;
        std::ipc::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline std::u32 remove(std::u32 pid, const char* path) {
        std::u32 len = 0;
        while (path[len] != '\0') len++;
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::VfsRemove;
        std::ipc::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline std::u32 open(std::u32 pid, const char* path, bool create = false) {
        std::u32 len = 0;
        while (path[len] != '\0') len++;
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::VfsOpen;
        msg.arg1 = create ? 1 : 0;
        std::ipc::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline std::i32 read(std::u32 pid, std::u32 handle, std::u32 offset, std::u8* buf,
                           std::u32 bufLen) {
        // Zero the buffer to avoid leaking uninitialized stack contents
        // across address spaces. The VFS ignores incoming data for reads
        // and replies with file contents in the same buffer.
        for (std::u32 i = 0; i < bufLen; i++) buf[i] = 0;
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::VfsRead;
        msg.arg1 = handle;
        msg.arg2 = offset;
        msg.arg3 = bufLen;
        std::ipc::send(pid, &msg, buf, bufLen);
        return static_cast<std::i32>(msg.arg1);
    }

    static inline std::u32 write(std::u32 pid, std::u32 handle, const std::u8* data, std::u32 len) {
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::VfsWrite;
        msg.arg1 = handle;
        msg.arg2 = len;
        std::ipc::send(pid, &msg, data, len);
        return msg.arg1;
    }

    // Returns 0 = not found, 1 = file, 2 = directory.
    static inline std::u32 stat(std::u32 pid, const char* path) {
        std::u32 len = 0;
        while (path[len] != '\0') len++;
        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::VfsStat;
        std::ipc::send(pid, &msg, path, len + 1);
        return msg.arg1;
    }

    static inline bool list(std::u32 pid, const char* path, std::u32 index,
                            char* nameOut, std::u32 nameMax) {
        std::u32 pathLen = 0;
        while (path[pathLen] != '\0') pathLen++;

        // Use a local buffer: send the path, reply overwrites with the name.
        // Buffer is one byte larger than SHELL_MAX_PATH to ensure null
        // termination at maximum path length.
        char buf[65];
        for (std::u32 k = 0; k < sizeof(buf); k++) buf[k] = 0;
        std::u32 i = 0;
        while (i < sizeof(buf) - 1 && i <= pathLen) {
            buf[i] = path[i];
            i++;
        }

        std::ipc::Message msg = {};
        msg.type = std::ipc::MessageType::VfsList;
        msg.arg1 = index;
        msg.arg2 = pathLen + 1;
        std::ipc::send(pid, &msg, buf, sizeof(buf));

        if (msg.arg1 == 0) {
            return false;
        }

        // buf now contains the reply name.
        i = 0;
        std::u32 limit = nameMax - 1;
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
