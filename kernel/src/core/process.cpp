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
    : currentPid(0), nextPid(1) {
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        processes[i].pid = 0;
        processes[i].state = ProcessState::Empty;
    }
}

bool Process::sendQueuePush(u32 senderPid) {
    if (sendQueueCount >= SEND_QUEUE_SIZE) {
        return false;
    }
    u32 idx = (sendQueueHead + sendQueueCount) % SEND_QUEUE_SIZE;
    sendQueue[idx] = senderPid;
    sendQueueCount++;
    return true;
}

u32 Process::sendQueuePop() {
    if (sendQueueCount == 0) {
        return 0;
    }
    u32 pid = sendQueue[sendQueueHead];
    sendQueueHead = (sendQueueHead + 1) % SEND_QUEUE_SIZE;
    sendQueueCount--;
    return pid;
}

static void copyMsg(const Message& src, Message& dst) {
    dst.type = src.type;
    dst.arg1 = src.arg1;
    dst.arg2 = src.arg2;
    dst.arg3 = src.arg3;
    dst.arg4 = src.arg4;
    dst.arg5 = src.arg5;
}

bool Process::notifyPush(u32 senderPid, const Message& m) {
    u32 next = (notifyHead + 1) % NOTIFY_QUEUE_SIZE;
    if (next == notifyTail) {
        return false;
    }
    notifyQueue[notifyHead].senderPid = senderPid;
    copyMsg(m, notifyQueue[notifyHead].msg);
    notifyHead = next;
    return true;
}

bool Process::notifyPop(u32& senderPid, Message& m) {
    if (notifyHead == notifyTail) {
        return false;
    }
    senderPid = notifyQueue[notifyTail].senderPid;
    copyMsg(notifyQueue[notifyTail].msg, m);
    notifyTail = (notifyTail + 1) % NOTIFY_QUEUE_SIZE;
    return true;
}

Process* ProcessManager::create(u32 eip, u32 esp, u32 cs, u32 ds, u32 pageDirectory) {
    for (u32 i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].state == ProcessState::Empty) {
            Process& p = processes[i];
            p.pid = nextPid++;
            p.state = ProcessState::Ready;
            p.eip = eip;
            p.esp = esp;
            p.cs = cs;
            p.ds = ds;
            p.eax = 0;
            p.ebx = 0;
            p.ecx = 0;
            p.edx = 0;
            p.esi = 0;
            p.edi = 0;
            p.ebp = 0;
            p.eflags = 0x202;
            p.pageDirectory = pageDirectory;
            p.kernelEsp = 0;
            p.msg = {};
            p.msgPtr = 0;
            p.sendQueueHead = 0;
            p.sendQueueCount = 0;
            for (u32 j = 0; j < Process::SEND_QUEUE_SIZE; j++) {
                p.sendQueue[j] = 0;
            }
            p.notifyHead = 0;
            p.notifyTail = 0;
            return &p;
        }
    }
    return nullptr;
}

void ProcessManager::destroy(u32 pid) {
    for (u32 i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid && processes[i].state != ProcessState::Empty) {
            processes[i].state = ProcessState::Empty;
            return;
        }
    }
}

Process* ProcessManager::current() {
    return &processes[currentPid];
}

Process* ProcessManager::get(u32 pid) {
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == pid && processes[i].state != ProcessState::Empty) {
            return &processes[i];
        }
    }
    return nullptr;
}

void ProcessManager::setCurrent(Process* process) {
    for (u32 i = 0; i < MAX_PROCESSES; i++) {
        if (&processes[i] == process) {
            currentPid = i;
            return;
        }
    }
}
