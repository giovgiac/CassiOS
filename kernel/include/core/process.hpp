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

#include <types.hpp>
#include <message.hpp>
#include <list.hpp>

namespace cassio {
namespace kernel {

enum class ProcessState : u8 {
    Empty,
    Ready,
    Running,
    SendBlocked,
    ReceiveBlocked,
};

struct Process {
    // Linked list node for heap-backed IPC send queue.
    struct SendNode {
        u32 senderPid;
        SendNode* next;
    };

    // Linked list node for heap-backed IPC notification queue.
    struct NotifyNode {
        u32 senderPid;
        Message msg;
        u8* data;      // Heap-allocated copy of sender's bulk data (nullptr if none).
        u32 dataLen;   // Length of copied data.
        NotifyNode* next;
    };

    u32 pid;
    ProcessState state;
    Process* next;  // Intrusive list link for ProcessManager.

    u32 eax, ebx, ecx, edx;
    u32 esi, edi, ebp, esp;
    u32 eip;
    u32 eflags;
    u32 cs, ds;

    u32 pageDirectory;
    u32 kernelEsp;
    u32 heapBase;   // Initial heap address (set once at ELF load, for heap size).
    u32 heapBreak;  // Current top of process heap (page-aligned, for sbrk).

    // IPC state.
    Message msg;        // Outgoing message (sender) or staging buffer.
    u32 msgPtr;         // Userspace pointer: reply buffer (sender) or receive buffer (receiver).
    u32 dataPtr;        // Userspace pointer to bulk data buffer.
    u32 dataLen;        // Bulk data length (outgoing) or capacity (incoming).

    // Send queue: PIDs of processes waiting to send to this process.
    LinkedList<SendNode> sendQueue;

    bool sendQueuePush(u32 senderPid);
    u32 sendQueuePop();

    // Notification queue: fire-and-forget messages (no reply expected).
    LinkedList<NotifyNode> notifyQueue;

    bool notifyPush(u32 senderPid, const Message& msg,
                    const void* data = nullptr, u32 dataLen = 0);
    bool notifyPop(u32& senderPid, Message& msg,
                   void* dataDst = nullptr, u32 dataCapacity = 0);
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
    Process* create(u32 eip, u32 esp, u32 cs, u32 ds, u32 pageDirectory);

    /**
     * @brief Destroys a process, freeing its IPC queues and the process itself.
     *
     */
    void destroy(u32 pid);

    /**
     * @brief Returns the currently running process.
     *
     */
    Process* current();

    /**
     * @brief Looks up a process by PID.
     *
     */
    Process* get(u32 pid);

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
    u32 getProcessCount() const;

    /** Deleted Methods */
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

private:
    ProcessManager();

    static ProcessManager instance;

    Process kernelTask;
    LinkedList<Process> processes;
    Process* currentProcess;
    u32 nextPid;
};

} // kernel
} // cassio

#endif // CORE_PROCESS_HPP_
