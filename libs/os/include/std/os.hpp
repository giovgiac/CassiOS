/**
 * os.hpp -- OS interface: syscall numbers and the structures they return
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Shared by the kernel and the kernel tests. Rust code uses the same
 * definitions from libs/sys; keep the two in step.
 *
 */

#ifndef STD_OS_HPP
#define STD_OS_HPP

#include <std/types.hpp>

namespace std {
namespace os {

namespace syscall {
constexpr u32 Send = 0;
constexpr u32 Receive = 1;
constexpr u32 Reply = 2;
constexpr u32 IrqRegister = 3;
constexpr u32 Write = 4;
constexpr u32 Sleep = 5;
constexpr u32 Uptime = 6;
constexpr u32 Reboot = 7;
constexpr u32 Shutdown = 8;
constexpr u32 Exit = 9;
constexpr u32 MapDevice = 10;
constexpr u32 Notify = 11;
constexpr u32 MemInfo = 12;
constexpr u32 Sbrk = 13;
constexpr u32 ProcList = 14;
constexpr u32 Exec = 15;
constexpr u32 WaitPid = 16;
constexpr u32 FramebufferInfo = 17;
constexpr u32 Count = 18;
} // namespace syscall

/** System tick frequency in Hz. Used to convert uptime() ticks to time. */
constexpr u32 TICK_FREQUENCY = 1000;

/** Process information returned by procList(). */
struct ProcEntry {
    u32 pid;      ///< Process ID.
    u32 state;    ///< 1=Ready, 2=Running, 3=SendBlocked, 4=ReceiveBlocked, 5=WaitBlocked.
    u32 heapSize; ///< Heap size in bytes (0 if no heap).
};

/** Framebuffer information returned by framebufferInfo(). */
struct FramebufferInfo {
    u32 address; ///< Physical address of the framebuffer.
    u32 width;   ///< Width in pixels.
    u32 height;  ///< Height in pixels.
    u32 pitch;   ///< Bytes per scanline.
    u32 bpp;     ///< Bits per pixel.
};

} // namespace os
} // namespace std

#endif // STD_OS_HPP
