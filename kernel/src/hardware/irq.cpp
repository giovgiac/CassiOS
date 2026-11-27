/**
 * irq.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/irq.hpp"

#include "core/process.hpp"
#include "core/syscall.hpp"

using namespace cassio;
using namespace std;
using namespace cassio::hardware;
using namespace cassio::kernel;

IrqManager IrqManager::instance;

u32 process_deliver_irq(u32 pid, u8 irq) {
    Process* target = ProcessManager::getManager().get(pid);
    if (!target) {
        return 0;
    }
    if (target->state != ProcessState::ReceiveBlocked) {
        return 2;
    }

    // Write the notification into the receive buffer, which lives in the
    // target's address space.
    ipc::Message notification = {};
    notification.type = ipc::MessageType::IrqNotify;
    notification.arg1 = irq;
    ipc::Message* buf = (ipc::Message*)target->msgPtr;

    u32 currentCR3;
    asm volatile("mov %%cr3, %0" : "=r"(currentCR3));
    u32 targetPD = target->pageDirectory;
    if (targetPD != 0 && targetPD != currentCR3) {
        asm volatile("mov %0, %%cr3" : : "r"(targetPD));
        *buf = notification;
        asm volatile("mov %0, %%cr3" : : "r"(currentCR3));
    } else {
        *buf = notification;
    }

    // receive() returns 0 (a kernel notification).
    SyscallFrame* frame = (SyscallFrame*)target->esp;
    frame->eax = 0;
    target->state = ProcessState::Ready;
    return 1;
}
