/**
 * process.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/process.hpp"
#include "core/syscall.hpp"
#include <memory.hpp>

using namespace cassio;
using namespace cassio::kernel;

ProcessManager ProcessManager::instance;

ProcessManager::ProcessManager()
    : currentProcess(&kernelTask), nextPid(1) {
    kernelTask.pid = 0;
    kernelTask.state = ProcessState::Empty;
    kernelTask.next = nullptr;
    kernelTask.eax = 0;
    kernelTask.ebx = 0;
    kernelTask.ecx = 0;
    kernelTask.edx = 0;
    kernelTask.esi = 0;
    kernelTask.edi = 0;
    kernelTask.ebp = 0;
    kernelTask.esp = 0;
    kernelTask.eip = 0;
    kernelTask.eflags = 0;
    kernelTask.cs = 0;
    kernelTask.ds = 0;
    kernelTask.pageDirectory = 0;
    kernelTask.kernelEsp = 0;
    kernelTask.heapBreak = 0;
    kernelTask.msg = {};
    kernelTask.msgPtr = 0;
    kernelTask.dataPtr = 0;
    kernelTask.dataLen = 0;
    processes.pushBack(&kernelTask);
}

// -- Send queue (heap-backed linked list) --

bool Process::sendQueuePush(u32 senderPid) {
    SendNode* node = new SendNode;
    if (!node) {
        return false;
    }
    node->senderPid = senderPid;
    sendQueue.pushBack(node);
    return true;
}

u32 Process::sendQueuePop() {
    SendNode* node = sendQueue.popFront();
    if (!node) {
        return 0;
    }
    u32 pid = node->senderPid;
    delete node;
    return pid;
}

// -- Notification queue (heap-backed linked list) --

static void copyMsg(const Message& src, Message& dst) {
    memcpy(&dst, &src, sizeof(Message));
}

bool Process::notifyPush(u32 senderPid, const Message& m,
                         const void* data, u32 dataLen) {
    NotifyNode* node = new NotifyNode;
    if (!node) {
        return false;
    }
    node->senderPid = senderPid;
    copyMsg(m, node->msg);
    node->data = nullptr;
    node->dataLen = 0;
    if (data != nullptr && dataLen > 0) {
        node->data = static_cast<u8*>(operator new(dataLen));
        if (!node->data) {
            delete node;
            return false;
        }
        memcpy(node->data, data, dataLen);
        node->dataLen = dataLen;
    }
    notifyQueue.pushBack(node);
    return true;
}

bool Process::notifyPop(u32& senderPid, Message& m,
                        void* dataDst, u32 dataCapacity) {
    NotifyNode* node = notifyQueue.popFront();
    if (!node) {
        return false;
    }
    senderPid = node->senderPid;
    copyMsg(node->msg, m);
    if (node->data != nullptr && node->dataLen > 0 &&
        dataDst != nullptr && dataCapacity > 0) {
        u32 copyLen = node->dataLen < dataCapacity ? node->dataLen : dataCapacity;
        memcpy(dataDst, node->data, copyLen);
    }
    operator delete(node->data);
    delete node;
    return true;
}

// -- ProcessManager --

Process* ProcessManager::create(u32 eip, u32 esp, u32 cs, u32 ds, u32 pageDirectory) {
    Process* p = new Process;
    if (!p) {
        return nullptr;
    }

    p->pid = nextPid++;
    p->state = ProcessState::Ready;
    p->next = nullptr;
    p->eip = eip;
    p->esp = esp;
    p->cs = cs;
    p->ds = ds;
    p->eax = 0;
    p->ebx = 0;
    p->ecx = 0;
    p->edx = 0;
    p->esi = 0;
    p->edi = 0;
    p->ebp = 0;
    p->eflags = 0x202;
    p->pageDirectory = pageDirectory;
    p->kernelEsp = 0;
    p->heapBreak = 0;
    p->msg = {};
    p->msgPtr = 0;
    p->dataPtr = 0;
    p->dataLen = 0;

    processes.pushBack(p);
    return p;
}

void ProcessManager::destroy(u32 pid) {
    if (pid == 0) {
        return;
    }

    // Find the process by PID.
    Process* p = processes.getHead();
    while (p) {
        if (p->pid == pid && p->state != ProcessState::Empty) {
            break;
        }
        p = p->next;
    }
    if (!p) {
        return;
    }

    // Wake all senders blocked on this process, returning -1 (error).
    while (!p->sendQueue.isEmpty()) {
        Process::SendNode* node = p->sendQueue.popFront();
        Process* sender = get(node->senderPid);
        if (sender && sender->state == ProcessState::SendBlocked) {
            SyscallFrame* frame = (SyscallFrame*)sender->esp;
            frame->eax = static_cast<u32>(-1);
            sender->state = ProcessState::Ready;
        }
        delete node;
    }

    // Drain notify queue.
    while (!p->notifyQueue.isEmpty()) {
        Process::NotifyNode* node = p->notifyQueue.popFront();
        operator delete(node->data);
        delete node;
    }

    // Unlink from process list and free.
    processes.remove(p);
    delete p;
}

Process* ProcessManager::current() {
    return currentProcess;
}

Process* ProcessManager::get(u32 pid) {
    Process* p = processes.getHead();
    while (p) {
        if (p->pid == pid && p->state != ProcessState::Empty) {
            return p;
        }
        p = p->next;
    }
    return nullptr;
}

void ProcessManager::setCurrent(Process* process) {
    currentProcess = process;
}

Process* ProcessManager::getHead() {
    return processes.getHead();
}

u32 ProcessManager::getProcessCount() const {
    return processes.getCount();
}
