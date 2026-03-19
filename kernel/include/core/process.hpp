/**
 * process.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_PROCESS_HPP_
#define CORE_PROCESS_HPP_

#include <std/types.hpp>
#include <std/ipc.hpp>
#include <std/collections/list.hpp>

namespace cassio {
namespace kernel {

enum class ProcessState : std::u8 {
    Empty,
    Ready,
    Running,
    SendBlocked,
    ReceiveBlocked,
};

struct Process {
    // Linked list node for heap-backed IPC send queue.
    struct SendNode {
        std::u32 senderPid;
        SendNode* next;
    };

    // Linked list node for heap-backed IPC notification queue.
    struct NotifyNode {
        std::u32 senderPid;
        std::ipc::Message msg;
        std::u8* data;      // Heap-allocated copy of sender's bulk data (nullptr if none).
        std::u32 dataLen;   // Length of copied data.
        NotifyNode* next;
    };

    std::u32 pid;
    ProcessState state;
    Process* next;  // Intrusive list link for ProcessManager.

    std::u32 eax, ebx, ecx, edx;
    std::u32 esi, edi, ebp, esp;
    std::u32 eip;
    std::u32 eflags;
    std::u32 cs, ds;

    std::u32 pageDirectory;
    std::u32 kernelEsp;
    std::u32 heapBase;   // Initial heap address (set once at ELF load, for heap size).
    std::u32 heapBreak;  // Current top of process heap (page-aligned, for sbrk).

    // IPC state.
    std::ipc::Message msg;        // Outgoing message (sender) or staging buffer.
    std::u32 msgPtr;         // Userspace pointer: reply buffer (sender) or receive buffer (receiver).
    std::u32 dataPtr;        // Userspace pointer to bulk data buffer.
    std::u32 dataLen;        // Bulk data length (outgoing) or capacity (incoming).

    // Send queue: PIDs of processes waiting to send to this process.
    std::collections::LinkedList<SendNode> sendQueue;

    bool sendQueuePush(std::u32 senderPid);
    std::u32 sendQueuePop();

    // Notification queue: fire-and-forget messages (no reply expected).
    std::collections::LinkedList<NotifyNode> notifyQueue;

    bool notifyPush(std::u32 senderPid, const std::ipc::Message& msg,
                    const void* data = nullptr, std::u32 dataLen = 0);
    bool notifyPop(std::u32& senderPid, std::ipc::Message& msg,
                   void* dataDst = nullptr, std::u32 dataCapacity = 0);
};

/**
 * @brief Singleton that manages the process table as a heap-backed linked list.
 *
 * PID 0 is reserved for the kernel task and is stored as an embedded member
 * (initialized before the heap is ready). All other processes are heap-allocated.
 *
 */
class ProcessManager {
public:
    inline static ProcessManager& getManager() {
        return instance;
    }

    /**
     * @brief Creates a new process via heap allocation.
     *
     * Returns null if the heap is exhausted.
     *
     */
    Process* create(std::u32 eip, std::u32 esp, std::u32 cs, std::u32 ds, std::u32 pageDirectory);

    /**
     * @brief Destroys a process, freeing its IPC queues and the process itself.
     *
     */
    void destroy(std::u32 pid);

    /**
     * @brief Returns the currently running process.
     *
     */
    Process* current();

    /**
     * @brief Looks up a process by PID.
     *
     */
    Process* get(std::u32 pid);

    /**
     * @brief Sets the given process as the current one.
     *
     */
    void setCurrent(Process* process);

    /**
     * @brief Returns the head of the process list (for scheduler iteration).
     *
     */
    Process* getHead();

    /**
     * @brief Returns the total number of processes (including kernel task).
     *
     */
    std::u32 getProcessCount() const;

    /** Deleted Methods */
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

private:
    ProcessManager();

    static ProcessManager instance;

    Process kernelTask;
    std::collections::LinkedList<Process> processes;
    Process* currentProcess;
    std::u32 nextPid;
};

} // kernel
} // cassio

#endif // CORE_PROCESS_HPP_
