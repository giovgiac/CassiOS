/**
 * vga.hpp -- VGA terminal service client helpers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef USERSPACE_LIB_VGA_HPP_
#define USERSPACE_LIB_VGA_HPP_

#include <std/types.hpp>
#include <message.hpp>
#include <ipc.hpp>

namespace cassio {

class Vga {
public:
    static inline void putchar(std::u32 pid, char ch) {
        Message msg = {};
        msg.type = MessageType::VgaPutchar;
        msg.arg1 = static_cast<std::u8>(ch);
        IPC::notify(pid, &msg);
    }

    static inline void write(std::u32 pid, const char* str) {
        std::u32 len = 0;
        while (str[len] != '\0') len++;
        Message msg = {};
        msg.type = MessageType::VgaWrite;
        msg.arg1 = len;
        IPC::notify(pid, &msg, str, len);
    }

    static inline void clear(std::u32 pid) {
        Message msg = {};
        msg.type = MessageType::VgaClear;
        IPC::send(pid, &msg);
    }

    static inline void setCursor(std::u32 pid, std::u8 col, std::u8 row) {
        Message msg = {};
        msg.type = MessageType::VgaSetCursor;
        msg.arg1 = col;
        msg.arg2 = row;
        IPC::send(pid, &msg);
    }

    static inline void getCursor(std::u32 pid, std::u8& col, std::u8& row) {
        Message msg = {};
        msg.type = MessageType::VgaGetCursor;
        IPC::send(pid, &msg);
        col = static_cast<std::u8>(msg.arg1);
        row = static_cast<std::u8>(msg.arg2);
    }
};

} // cassio

#endif // USERSPACE_LIB_VGA_HPP_
