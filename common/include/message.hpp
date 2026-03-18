/**
 * message.hpp -- IPC message format and type constants
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_MESSAGE_HPP_
#define COMMON_MESSAGE_HPP_

#include <types.hpp>

namespace cassio {

struct Message {
    u32 type;
    u32 arg1;
    u32 arg2;
    u32 arg3;
    u32 arg4;
    u32 arg5;
};

namespace MessageType {
    // Kernel.
    constexpr u32 IrqNotify    = 1;

    // Nameserver.
    constexpr u32 NsRegister   = 2;
    constexpr u32 NsLookup     = 3;

    // Keyboard.
    constexpr u32 KbdRead      = 4;

    // VGA.
    constexpr u32 VgaPutchar   = 5;
    constexpr u32 VgaWrite     = 6;
    constexpr u32 VgaClear     = 7;
    constexpr u32 VgaSetCursor = 8;
    constexpr u32 VgaGetCursor = 9;

    // VFS.
    constexpr u32 VfsMkdir     = 10;
    constexpr u32 VfsRemove    = 11;
    constexpr u32 VfsOpen      = 12;
    constexpr u32 VfsRead      = 13;
    constexpr u32 VfsWrite     = 14;
    constexpr u32 VfsList      = 15;
    constexpr u32 VfsStat      = 16;

    // Mouse.
    constexpr u32 MouseRead    = 17;

    // ATA.
    constexpr u32 AtaRead      = 18;
    constexpr u32 AtaWrite     = 19;
}

} // cassio

#endif // COMMON_MESSAGE_HPP_
