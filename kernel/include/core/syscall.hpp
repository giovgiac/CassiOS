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

#include <std/types.hpp>
#include <std/os.hpp>
#include <std/ipc.hpp>

namespace cassio {
namespace kernel {

/**
 * @brief Layout of the saved register frame on the kernel stack after
 *        pusha + segment register pushes in all stubs (interrupt, exception,
 *        and syscall).
 *
 * Every stub pushes [error_code, number] before pusha for a uniform
 * stack layout, ensuring context switches work regardless of entry path.
 *
 */
struct SyscallFrame {
    std::u32 gs, fs, es, ds;
    std::u32 edi, esi, ebp, esp_pusha;
    std::u32 ebx, edx, ecx, eax;
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

    std::i32 write(std::u32 fd, std::u32 buf, std::u32 len);
    std::i32 sleep(std::u32 ms);
    std::i32 uptime();
    void reboot();
    void shutdown();
    void exit(std::u32 code);
    std::i32 mapDevice(std::u32 physical, std::u32 virt, std::u32 pages);
    void memInfo(std::u32& total, std::u32& used, std::u32& free);

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
    std::u32 handleSyscall(std::u32 esp);

    /**
     * @brief IPC send: sends msg to targetPid. Caller becomes SendBlocked.
     *
     * The msg pointer also serves as the reply buffer.
     * Returns 0 if caller should block, -1 on error.
     *
     */
    std::i32 send(std::u32 targetPid, std::ipc::Message* msg, std::u32 dataPtr, std::u32 dataLen);
    std::i32 notify(std::u32 targetPid, std::ipc::Message* msg, std::u32 dataPtr, std::u32 dataLen);

    /**
     * @brief IPC receive: receives a message into msg.
     *
     * Returns sender PID if a message was delivered immediately,
     * 0 if caller should block, -1 on error.
     *
     */
    std::i32 receive(std::ipc::Message* msg, std::u32 dataPtr, std::u32 dataCapacity);

    /**
     * @brief IPC reply: sends a reply to a SendBlocked process.
     *
     * Returns 0 on success, -1 on error.
     *
     */
    std::i32 reply(std::u32 targetPid, std::ipc::Message* msg, std::u32 dataPtr, std::u32 dataLen);

    /**
     * @brief Grows the calling process's heap by increment bytes.
     *
     * Returns the previous break address, or 0 on failure.
     *
     */
    std::u32 sbrk(std::u32 increment);

    /**
     * @brief Returns process info for all userspace processes.
     *
     * Writes up to maxEntries ProcEntry structs into buf, skipping PID 0.
     * Returns the number of entries written.
     *
     */
    std::u32 procList(std::os::ProcEntry* buf, std::u32 maxEntries);

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
extern "C" std::u32 handleSyscall(std::u32 esp);

#endif // CORE_SYSCALL_HPP_
