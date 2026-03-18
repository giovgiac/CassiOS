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

#include <std/types.hpp>

namespace cassio {

struct Message {
    std::u32 type;
    std::u32 arg1;
    std::u32 arg2;
    std::u32 arg3;
    std::u32 arg4;
    std::u32 arg5;
};

namespace MessageType {
    // Kernel.
    constexpr std::u32 IrqNotify    = 1;

    // Nameserver.
    constexpr std::u32 NsRegister   = 2;
    constexpr std::u32 NsLookup     = 3;

    // Keyboard.
    constexpr std::u32 KbdRead      = 4;

    // VGA.
    constexpr std::u32 VgaPutchar   = 5;
    constexpr std::u32 VgaWrite     = 6;
    constexpr std::u32 VgaClear     = 7;
    constexpr std::u32 VgaSetCursor = 8;
    constexpr std::u32 VgaGetCursor = 9;

    // VFS.
    constexpr std::u32 VfsMkdir     = 10;
    constexpr std::u32 VfsRemove    = 11;
    constexpr std::u32 VfsOpen      = 12;
    constexpr std::u32 VfsRead      = 13;
    constexpr std::u32 VfsWrite     = 14;
    constexpr std::u32 VfsList      = 15;
    constexpr std::u32 VfsStat      = 16;

    // Mouse.
    constexpr std::u32 MouseRead    = 17;

    // ATA.
    constexpr std::u32 AtaRead      = 18;
    constexpr std::u32 AtaWrite     = 19;

    // Nameserver (extended).
    constexpr std::u32 NsListAll    = 20;
}

} // cassio

#endif // COMMON_MESSAGE_HPP_
