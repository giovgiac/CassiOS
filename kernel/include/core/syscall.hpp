/**
 * syscall.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_SYSCALL_HPP_
#define CORE_SYSCALL_HPP_

#include <common/types.hpp>
#include <syscall.hpp>
#include <message.hpp>

namespace cassio {
namespace kernel {

/**
 * @brief Layout of the saved register frame on the kernel stack after
 *        pusha + segment register pushes in the syscall/interrupt stubs.
 *
 */
struct SyscallFrame {
    u32 gs, fs, es, ds;
    u32 edi, esi, ebp, esp_pusha;
    u32 ebx, edx, ecx, eax;
};

/**
 * @brief Singleton that handles int 0x80 syscall dispatch.
 *
 * Registers a DPL=3 trap gate at vector 0x80 and routes syscalls
 * to handler methods based on the syscall number in EAX. Supports
 * blocking IPC syscalls via context switching.
 *
 */
class SyscallHandler {
private:
    static SyscallHandler instance;

private:
    SyscallHandler();
    ~SyscallHandler() = default;

    i32 write(u32 fd, u32 buf, u32 len);
    i32 sleep(u32 ms);
    i32 uptime();
    void reboot();
    void shutdown();

public:
    /**
     * @brief Returns the singleton SyscallHandler instance.
     *
     */
    inline static SyscallHandler& getSyscallHandler() {
        return instance;
    }

    /**
     * @brief Registers the syscall gate at vector 0x80 with DPL=3.
     *
     */
    void load();

    /**
     * @brief Dispatches a syscall from the saved register frame.
     *
     * Returns the ESP to restore (may differ from input if the caller
     * blocks and a context switch occurs).
     *
     */
    u32 handleSyscall(u32 esp);

    /**
     * @brief IPC send: sends msg to targetPid. Caller becomes SendBlocked.
     *
     * The msg pointer also serves as the reply buffer.
     * Returns 0 if caller should block, -1 on error.
     *
     */
    i32 send(u32 targetPid, Message* msg);

    /**
     * @brief IPC receive: receives a message into msg.
     *
     * Returns sender PID if a message was delivered immediately,
     * 0 if caller should block, -1 on error.
     *
     */
    i32 receive(Message* msg);

    /**
     * @brief IPC reply: sends a reply to a SendBlocked process.
     *
     * Returns 0 on success, -1 on error.
     *
     */
    i32 reply(u32 targetPid, Message* msg);

    /** Deleted Methods */
    SyscallHandler(const SyscallHandler&) = delete;
    SyscallHandler(SyscallHandler&&) = delete;
    SyscallHandler& operator=(const SyscallHandler&) = delete;
    SyscallHandler& operator=(SyscallHandler&&) = delete;
};

} // kernel
} // cassio

/**
 * @brief C-linkage syscall handler called from the assembly stub in syscall_stub.s.
 *
 * Takes the saved ESP and returns the ESP to restore (may differ on context switch).
 *
 */
extern "C" cassio::u32 handleSyscall(cassio::u32 esp);

#endif // CORE_SYSCALL_HPP_
