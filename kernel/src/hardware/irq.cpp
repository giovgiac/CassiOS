/**
 * irq.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/irq.hpp"
#include "hardware/driver.hpp"
#include "hardware/interrupt.hpp"
#include "core/process.hpp"
#include "core/syscall.hpp"

using namespace cassio;
using namespace cassio::hardware;

IrqManager IrqManager::instance;

// drv[] is not zeroed here -- BSS guarantees zero-initialization before
// any constructors run, and drivers may register before this constructor
// executes (static initialization order).
IrqManager::IrqManager()
    : pic_master_cmd(PortType::MasterProgrammableInterfaceControllerCommand),
      pic_master_data(PortType::MasterProgrammableInterfaceControllerData),
      pic_slave_cmd(PortType::SlaveProgrammableInterfaceControllerCommand),
      pic_slave_data(PortType::SlaveProgrammableInterfaceControllerData) {}

void IrqManager::load() {
    InterruptManager& im = InterruptManager::getManager();
    im.setInterruptGate(0x20, &handleInterruptRequest0x00);
    im.setInterruptGate(0x21, &handleInterruptRequest0x01);
    im.setInterruptGate(0x2C, &handleInterruptRequest0x0C);
    im.setInterruptGate(0x2E, &handleInterruptRequest0x0E);

    // Remap the PICs: master starts at 0x20, slave at 0x28.
    pic_master_cmd.writeSlow(0x11);
    pic_slave_cmd.writeSlow(0x11);

    pic_master_data.writeSlow(IRQ_OFFSET);
    pic_slave_data.writeSlow(IRQ_OFFSET + 8);

    pic_master_data.writeSlow(0x04);
    pic_slave_data.writeSlow(0x02);

    pic_master_data.writeSlow(0x01);
    pic_slave_data.writeSlow(0x01);

    pic_master_data.writeSlow(0x00);
    pic_slave_data.writeSlow(0x00);
}

u32 IrqManager::handleIrq(u8 number, u32 esp) {
    u8 irq = number - IRQ_OFFSET;

    if (irq < 16 && drv[irq] != nullptr) {
        esp = drv[irq]->handleInterrupt(esp);
    }

    // Send EOI to master PIC.
    pic_master_cmd.writeSlow(0x20);

    // If the IRQ came from the slave PIC, send EOI to slave too.
    if (irq >= 8) {
        pic_slave_cmd.writeSlow(0x20);
    }

    // Forward to userspace if a process is registered for this IRQ.
    if (irq < 16 && forwardPid[irq] != 0) {
        kernel::Process* target =
            kernel::ProcessManager::getManager().get(forwardPid[irq]);
        if (target && target->state == kernel::ProcessState::ReceiveBlocked) {
            // Deliver IrqNotify message directly to the waiting process.
            Message* buf = (Message*)target->msgPtr;
            buf->type = MessageType::IrqNotify;
            buf->arg1 = irq;
            buf->arg2 = 0;
            buf->arg3 = 0;
            buf->arg4 = 0;
            buf->arg5 = 0;

            // Set receive() return value to 0 (kernel notification).
            kernel::SyscallFrame* frame = (kernel::SyscallFrame*)target->esp;
            frame->eax = 0;

            target->state = kernel::ProcessState::Ready;
        } else if (target) {
            pendingIrq[irq] = true;
        }
    }

    return esp;
}

i32 IrqManager::registerForward(u8 irq, u32 pid) {
    if (irq >= 16) {
        return -1;
    }
    forwardPid[irq] = pid;
    return 0;
}

bool IrqManager::deliverPending(u32 pid, Message* msg) {
    for (u8 irq = 0; irq < 16; irq++) {
        if (forwardPid[irq] == pid && pendingIrq[irq]) {
            pendingIrq[irq] = false;
            msg->type = MessageType::IrqNotify;
            msg->arg1 = irq;
            msg->arg2 = 0;
            msg->arg3 = 0;
            msg->arg4 = 0;
            msg->arg5 = 0;
            return true;
        }
    }
    return false;
}

void IrqManager::registerDriver(u8 vector, Driver* driver) {
    u8 irq = vector - IRQ_OFFSET;
    if (irq < 16) {
        drv[irq] = driver;
    }
}

void IrqManager::unregisterDriver(u8 vector, Driver* driver) {
    u8 irq = vector - IRQ_OFFSET;
    if (irq < 16 && drv[irq] == driver) {
        drv[irq] = nullptr;
    }
}

u32 handleInterrupt(u8 number, u32 esp) {
    IrqManager& irq = IrqManager::getManager();
    return irq.handleIrq(number, esp);
}
