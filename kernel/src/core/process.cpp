/**
 * process.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/process.hpp"

using namespace cassio;
using namespace cassio::kernel;

ProcessManager ProcessManager::instance;

ProcessManager::ProcessManager()
    : currentProcess(&kernelTask), nextPid(1), processCount(1) {
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
    kernelTask.sendHead = nullptr;
    kernelTask.sendTail = nullptr;
    kernelTask.sendQueueCount = 0;
    kernelTask.notifyHead = nullptr;
    kernelTask.notifyTail = nullptr;
    head = &kernelTask;
}

// -- Send queue (heap-backed linked list) --

bool Process::sendQueuePush(u32 senderPid) {
    SendNode* node = new SendNode;
    if (!node) {
        return false;
    }
    node->senderPid = senderPid;
    node->next = nullptr;
    if (sendTail) {
        sendTail->next = node;
    } else {
        sendHead = node;
    }
    sendTail = node;
    sendQueueCount++;
    return true;
}

u32 Process::sendQueuePop() {
    if (!sendHead) {
        return 0;
    }
    SendNode* node = sendHead;
    u32 pid = node->senderPid;
    sendHead = node->next;
    if (!sendHead) {
        sendTail = nullptr;
    }
    sendQueueCount--;
    delete node;
    return pid;
}

// -- Notification queue (heap-backed linked list) --

static void copyMsg(const Message& src, Message& dst) {
    dst.type = src.type;
    dst.arg1 = src.arg1;
    dst.arg2 = src.arg2;
    dst.arg3 = src.arg3;
    dst.arg4 = src.arg4;
    dst.arg5 = src.arg5;
}

bool Process::notifyPush(u32 senderPid, const Message& m) {
    NotifyNode* node = new NotifyNode;
    if (!node) {
        return false;
    }
    node->senderPid = senderPid;
    copyMsg(m, node->msg);
    node->next = nullptr;
    if (notifyTail) {
        notifyTail->next = node;
    } else {
        notifyHead = node;
    }
    notifyTail = node;
    return true;
}

bool Process::notifyPop(u32& senderPid, Message& m) {
    if (!notifyHead) {
        return false;
    }
    NotifyNode* node = notifyHead;
    senderPid = node->senderPid;
    copyMsg(node->msg, m);
    notifyHead = node->next;
    if (!notifyHead) {
        notifyTail = nullptr;
    }
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
    p->sendHead = nullptr;
    p->sendTail = nullptr;
    p->sendQueueCount = 0;
    p->notifyHead = nullptr;
    p->notifyTail = nullptr;

    // Append to end of list.
    Process* tail = head;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = p;
    processCount++;

    return p;
}

void ProcessManager::destroy(u32 pid) {
    if (pid == 0) {
        return;
    }

    Process* prev = nullptr;
    Process* p = head;
    while (p) {
        if (p->pid == pid && p->state != ProcessState::Empty) {
            // Drain send queue.
            while (p->sendHead) {
                Process::SendNode* node = p->sendHead;
                p->sendHead = node->next;
                delete node;
            }
            p->sendTail = nullptr;
            p->sendQueueCount = 0;

            // Drain notify queue.
            while (p->notifyHead) {
                Process::NotifyNode* node = p->notifyHead;
                p->notifyHead = node->next;
                delete node;
            }
            p->notifyTail = nullptr;

            // Unlink from list.
            if (prev) {
                prev->next = p->next;
            } else {
                head = p->next;
            }

            processCount--;
            delete p;
            return;
        }
        prev = p;
        p = p->next;
    }
}

Process* ProcessManager::current() {
    return currentProcess;
}

Process* ProcessManager::get(u32 pid) {
    Process* p = head;
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
    return head;
}

u32 ProcessManager::getProcessCount() const {
    return processCount;
}
