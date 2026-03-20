/**
 * os.hpp -- OS interface: syscall constants and userspace wrappers
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Syscall number constants (os::syscall) are shared between kernel
 * and userspace. The free functions provide userspace wrappers
 * around int 0x80.
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

/**
 * Write bytes to a file descriptor.
 * fd 1 = VGA (removed), fd 2 = serial.
 * Returns the number of bytes written, or negative on error.
 */
i32 write(u32 fd, const char* buf, u32 len);

/**
 * Suspend the calling process for at least @p ms milliseconds.
 * Returns 0 on success.
 */
i32 sleep(u32 ms);

/**
 * Return the number of PIT ticks since boot.
 * Divide by os::TICK_FREQUENCY to get seconds.
 */
i32 uptime();

/** Reboot the system immediately. Does not return. */
void reboot();

/** Halt the system immediately. Does not return. */
void shutdown();

/**
 * Query physical memory statistics.
 * Values are returned as frame counts (1 frame = 4 KiB).
 */
void memInfo(u32& total, u32& used, u32& free);

/** Terminate the calling process with the given exit code. Does not return. */
void exit(u32 code);

/**
 * Register the calling process to receive IRQ @p irq via IPC.
 * Returns 0 on success, negative on error (e.g., IRQ already registered).
 */
i32 irqRegister(u32 irq);

/**
 * Map @p pages physical pages starting at @p physical into the
 * calling process's address space at virtual address @p virt.
 * Used by drivers to access memory-mapped hardware (e.g., VGA buffer).
 * Returns 0 on success.
 */
i32 mapDevice(u32 physical, u32 virt, u32 pages);

/**
 * Grow the calling process's heap by @p increment bytes.
 * Returns a pointer to the start of the new memory, or nullptr on failure.
 * The heap grows contiguously from the process's break address.
 */
void* sbrk(u32 increment);

/**
 * Fill @p buf with information about running processes (up to @p maxEntries).
 * Returns the number of entries written.
 */
u32 procList(ProcEntry* buf, u32 maxEntries);

/**
 * Load an ELF binary from memory and spawn it as a new process.
 * @p elfData points to the ELF file contents in the caller's address space.
 * @p size is the length in bytes.
 * Returns the child PID on success, or 0 on failure.
 */
u32 exec(const void* elfData, u32 size);

/**
 * Block until the process with @p pid exits.
 * Returns 0 on success, -1 if the PID does not exist.
 */
i32 waitpid(u32 pid);

/**
 * Return VESA framebuffer information from the multiboot info.
 * The display driver uses this to discover the framebuffer address
 * and resolution at startup.
 */
FramebufferInfo framebufferInfo();

} // namespace os
} // namespace std

#endif // STD_OS_HPP
