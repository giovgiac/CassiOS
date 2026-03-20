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

// Keyboard.
constexpr u32 KbdRead = 4;

// VFS.
constexpr u32 VfsMkdir = 10;
constexpr u32 VfsRemove = 11;
constexpr u32 VfsOpen = 12;
constexpr u32 VfsRead = 13;
constexpr u32 VfsWrite = 14;
constexpr u32 VfsList = 15;
constexpr u32 VfsStat = 16;

// Mouse.
constexpr u32 MouseRead = 17;

// ATA.
constexpr u32 AtaRead = 18;
constexpr u32 AtaWrite = 19;

// Nameserver (extended).
constexpr u32 NsListAll = 20;

// Display.
constexpr u32 DisplayFillRect = 21;
constexpr u32 DisplayDrawRect = 22;
constexpr u32 DisplayBlit = 23;
constexpr u32 DisplayScroll = 24;
constexpr u32 DisplayFlush = 25;
constexpr u32 DisplayGetInfo = 26;
constexpr u32 DisplayDrawChar = 27;

// Terminal.
constexpr u32 TerminalPutchar = 28;
constexpr u32 TerminalWrite = 29;
constexpr u32 TerminalClear = 30;
constexpr u32 TerminalSetCursor = 31;
constexpr u32 TerminalGetCursor = 32;
constexpr u32 TerminalFlush = 33;
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
