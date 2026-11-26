/**
 * ipc.hpp -- Inter-process communication
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * IPC message format, message type constants, and userspace
 * communication primitives: synchronous send/receive/reply
 * and fire-and-forget notify.
 *
 */

#ifndef STD_IPC_HPP
#define STD_IPC_HPP

#include <std/types.hpp>

namespace std {
namespace ipc {

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
constexpr u32 IrqNotify = 1;

// Nameserver.
constexpr u32 NsRegister = 2;
constexpr u32 NsLookup = 3;
constexpr u32 NsListAll = 4;

// Keyboard.
constexpr u32 KbdRead = 5;

// VFS.
constexpr u32 VfsMkdir = 6;
constexpr u32 VfsRemove = 7;
constexpr u32 VfsOpen = 8;
constexpr u32 VfsRead = 9;
constexpr u32 VfsWrite = 10;
constexpr u32 VfsList = 11;
constexpr u32 VfsStat = 12;
constexpr u32 VfsClose = 13;

// Mouse.
constexpr u32 MouseRead = 14;

// ATA.
constexpr u32 AtaRead = 15;
constexpr u32 AtaWrite = 16;

// Display.
constexpr u32 DisplayFillRect = 17;
constexpr u32 DisplayDrawRect = 18;
constexpr u32 DisplayBlit = 19;
constexpr u32 DisplayScroll = 20;
constexpr u32 DisplayFlush = 21;
constexpr u32 DisplayGetInfo = 22;
constexpr u32 DisplayDrawChar = 23;

// Terminal.
constexpr u32 TerminalPutchar = 24;
constexpr u32 TerminalWrite = 25;
constexpr u32 TerminalClear = 26;
constexpr u32 TerminalSetCursor = 27;
constexpr u32 TerminalGetCursor = 28;
constexpr u32 TerminalFlush = 29;
} // namespace MessageType

/**
 * Send a message to process @p pid and block until it replies.
 * The reply overwrites @p msg in place.
 * Returns the sender PID on success, negative on error.
 */
i32 send(u32 pid, Message* msg, const void* data = nullptr, u32 dataLen = 0);

/**
 * Block until a message arrives. The message is written into @p msg.
 * Optional @p data buffer receives bulk data (up to @p dataCapacity bytes).
 * Returns the sender PID, or negative on error.
 */
i32 receive(Message* msg, void* data = nullptr, u32 dataCapacity = 0);

/**
 * Reply to a blocked sender at @p pid. Unblocks the sender.
 * Returns 0 on success, negative on error.
 */
i32 reply(u32 pid, Message* msg, const void* data = nullptr, u32 dataLen = 0);

/**
 * Fire-and-forget: enqueue a message to @p pid without blocking.
 * Returns 0 on success, negative if the queue is full.
 */
i32 notify(u32 pid, Message* msg, const void* data = nullptr, u32 dataLen = 0);

} // namespace ipc
} // namespace std

#endif // STD_IPC_HPP
