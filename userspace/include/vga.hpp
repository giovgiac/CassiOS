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

#include <types.hpp>
#include <message.hpp>
#include <ipc.hpp>

namespace cassio {

class Vga {
public:
    static inline void putchar(u32 pid, char ch) {
        Message msg = {};
        msg.type = MessageType::VgaPutchar;
        msg.arg1 = static_cast<u8>(ch);
        IPC::notify(pid, &msg);
    }

    static inline void write(u32 pid, const char* str) {
        while (*str != '\0') {
            Message msg = {};
            msg.type = MessageType::VgaWrite;
            char* data = reinterpret_cast<char*>(&msg.arg1);
            u32 i = 0;
            while (i < 20 && str[i] != '\0') {
                data[i] = str[i];
                ++i;
            }
            if (i < 20) {
                data[i] = '\0';
            }
            IPC::notify(pid, &msg);
            str += i;
        }
    }

    static inline void clear(u32 pid) {
        Message msg = {};
        msg.type = MessageType::VgaClear;
        IPC::send(pid, &msg);
    }

    static inline void setCursor(u32 pid, u8 col, u8 row) {
        Message msg = {};
        msg.type = MessageType::VgaSetCursor;
        msg.arg1 = col;
        msg.arg2 = row;
        IPC::send(pid, &msg);
    }
};

} // cassio

#endif // USERSPACE_LIB_VGA_HPP_
