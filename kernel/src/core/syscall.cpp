/**
 * syscall.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/syscall.hpp"
#include "core/process.hpp"
#include "core/scheduler.hpp"
#include "hardware/pit.hpp"
#include "hardware/interrupt.hpp"
#include "hardware/irq.hpp"
#include "hardware/port.hpp"
#include "hardware/serial.hpp"
#include "memory/paging.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace cassio::hardware;
using namespace cassio::kernel;

extern "C" void syscallEntry();

SyscallHandler SyscallHandler::instance;

SyscallHandler::SyscallHandler() {}

void SyscallHandler::load() {
    InterruptManager& im = InterruptManager::getManager();
    im.setTrapGate(0x80, &syscallEntry, 3);
}

static void copyMessage(const Message* src, Message* dst) {
    dst->type = src->type;
    dst->arg1 = src->arg1;
    dst->arg2 = src->arg2;
    dst->arg3 = src->arg3;
    dst->arg4 = src->arg4;
    dst->arg5 = src->arg5;
}

// Copy a message to a userspace buffer that belongs to a different process.
// Temporarily switches page directory so the target's virtual address resolves
// to the correct physical page.
static void copyMessageToProcess(Process* target, Message* dst, const Message* src) {
    u32 currentCR3;
    asm volatile("mov %%cr3, %0" : "=r"(currentCR3));

    u32 targetPD = target->pageDirectory;
    if (targetPD != 0 && targetPD != currentCR3) {
        asm volatile("mov %0, %%cr3" : : "r"(targetPD));
        copyMessage(src, dst);
        asm volatile("mov %0, %%cr3" : : "r"(currentCR3));
    } else {
        copyMessage(src, dst);
    }
}

i32 SyscallHandler::send(u32 targetPid, Message* msg) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* sender = pm.current();
    Process* target = pm.get(targetPid);

    if (!target || target == sender) {
        return -1;
    }

    // Copy outgoing message into sender's process struct.
    copyMessage(msg, &sender->msg);
    sender->msgPtr = (u32)msg;  // Reply will be written here.

    if (target->state == ProcessState::ReceiveBlocked) {
        // Target is waiting for a message -- deliver immediately.
        Message* targetBuf = (Message*)target->msgPtr;
        copyMessageToProcess(target, targetBuf, &sender->msg);

        // Set target's return value to sender PID.
        SyscallFrame* targetFrame = (SyscallFrame*)target->esp;
        targetFrame->eax = sender->pid;

        target->state = ProcessState::Ready;
    } else {
        // Target is busy -- enqueue sender.
        if (!target->sendQueuePush(sender->pid)) {
            return -1;  // Queue full.
        }
    }

    sender->state = ProcessState::SendBlocked;
    return 0;  // Caller should block.
}

i32 SyscallHandler::receive(Message* msg) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* receiver = pm.current();

    // Check for pending IRQ notifications first.
    IrqManager& irqMgr = IrqManager::getManager();
    if (irqMgr.deliverPending(receiver->pid, msg)) {
        return -2;  // IRQ notification delivered; handleSyscall sets eax=0.
    }

    // Drain queued notifications before blocked senders so that
    // fire-and-forget messages are processed in chronological order
    // relative to blocking sends from the same source.
    u32 notifySender;
    if (receiver->notifyPop(notifySender, *msg)) {
        return -3;
    }

    if (receiver->sendQueueCount > 0) {
        // Pending sender -- deliver immediately.
        u32 senderPid = receiver->sendQueuePop();
        Process* sender = pm.get(senderPid);
        if (!sender) {
            return -1;
        }

        copyMessage(&sender->msg, msg);
        return static_cast<i32>(senderPid);
    }

    // No pending IRQs, notifications, or senders -- block.
    receiver->msgPtr = (u32)msg;
    receiver->state = ProcessState::ReceiveBlocked;
    return 0;  // Caller should block.
}

i32 SyscallHandler::reply(u32 targetPid, Message* msg) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* target = pm.get(targetPid);

    if (!target || target->state != ProcessState::SendBlocked) {
        return -1;
    }

    // Copy reply to kernel space first, then to sender's userspace buffer.
    // The msg pointer is in the replier's address space and becomes invalid
    // after switching to the sender's page directory.
    Message replyBuf;
    copyMessage(msg, &replyBuf);

    Message* senderBuf = (Message*)target->msgPtr;
    copyMessageToProcess(target, senderBuf, &replyBuf);

    // Set sender's return value to 0 (success).
    SyscallFrame* targetFrame = (SyscallFrame*)target->esp;
    targetFrame->eax = 0;

    target->state = ProcessState::Ready;
    return 0;
}

i32 SyscallHandler::notify(u32 targetPid, Message* msg) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* sender = pm.current();
    Process* target = pm.get(targetPid);

    if (!target || target == sender) {
        return -1;
    }

    // Copy message to kernel space (source is in caller's address space).
    Message temp;
    copyMessage(msg, &temp);

    if (target->state == ProcessState::ReceiveBlocked) {
        // Deliver immediately.
        Message* targetBuf = (Message*)target->msgPtr;
        copyMessageToProcess(target, targetBuf, &temp);

        // Return 0 as sender so receiver knows not to reply.
        SyscallFrame* targetFrame = (SyscallFrame*)target->esp;
        targetFrame->eax = 0;

        target->state = ProcessState::Ready;
    } else {
        // Queue for later delivery.
        if (!target->notifyPush(sender->pid, temp)) {
            return -1;
        }
    }

    return 0;  // Caller continues (no blocking).
}

i32 SyscallHandler::write(u32 fd, u32 buf, u32 len) {
    const char* str = reinterpret_cast<const char*>(buf);

    if (fd == 2) {
        Serial& serial = COM1::getSerial();
        for (u32 i = 0; i < len; ++i) {
            serial.putchar(str[i]);
        }
        return static_cast<i32>(len);
    }

    return -1;
}

i32 SyscallHandler::mapDevice(u32 physical, u32 virt, u32 pages) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* caller = pm.current();

    if (!caller || caller->pageDirectory == 0) {
        return -1;
    }

    memory::PagingManager& paging = memory::PagingManager::getManager();
    u16 flags = memory::PAGE_PRESENT | memory::PAGE_READWRITE | memory::PAGE_USER;

    for (u32 i = 0; i < pages; ++i) {
        paging.mapUserPage(caller->pageDirectory,
                           virt + i * 0x1000,
                           physical + i * 0x1000,
                           flags);
    }

    return 0;
}

i32 SyscallHandler::sleep(u32 ms) {
    PitTimer& pit = PitTimer::getTimer();
    pit.sleep(ms);
    return 0;
}

i32 SyscallHandler::uptime() {
    PitTimer& pit = PitTimer::getTimer();
    return static_cast<i32>(pit.getTicks());
}

void SyscallHandler::reboot() {
    Port<u8> kbCmd(PortType::KeyboardControllerCommand);
    kbCmd.write(0xFE);
}

void SyscallHandler::shutdown() {
    asm volatile("cli");
    asm volatile("hlt");
}

void SyscallHandler::memInfo(u32& total, u32& used, u32& free) {
    memory::PhysicalMemoryManager& pmm = memory::PhysicalMemoryManager::getManager();
    total = pmm.getTotalFrames();
    used = pmm.getUsedFrames();
    free = pmm.getFreeFrames();
}

void SyscallHandler::exit(u32 code) {
    Port<u8> debug_exit(PortType::QemuDebugExit);
    debug_exit.write(code == 0 ? 0x00 : 0x01);
}

u32 SyscallHandler::handleSyscall(u32 esp) {
    SyscallFrame* frame = (SyscallFrame*)esp;
    u32 number = frame->eax;

    switch (number) {
    case SyscallNumber::Send: {
        i32 result = send(frame->ebx, (Message*)frame->ecx);
        if (result == 0) {
            Scheduler& sched = Scheduler::getScheduler();
            return sched.reschedule(esp);
        }
        frame->eax = static_cast<u32>(result);
        return esp;
    }
    case SyscallNumber::Receive: {
        i32 result = receive((Message*)frame->ebx);
        if (result == 0) {
            Scheduler& sched = Scheduler::getScheduler();
            return sched.reschedule(esp);
        }
        if (result == -2 || result == -3) {
            // IRQ or queued notification delivered. Return 0 (no reply expected).
            frame->eax = 0;
            return esp;
        }
        frame->eax = static_cast<u32>(result);
        return esp;
    }
    case SyscallNumber::Reply: {
        i32 result = reply(frame->ebx, (Message*)frame->ecx);
        frame->eax = static_cast<u32>(result);
        return esp;
    }
    case SyscallNumber::IrqRegister: {
        IrqManager& irqMgr = IrqManager::getManager();
        ProcessManager& pm = ProcessManager::getManager();
        frame->eax = static_cast<u32>(irqMgr.registerForward(
            static_cast<u8>(frame->ebx), pm.current()->pid));
        return esp;
    }
    case SyscallNumber::Write:
        frame->eax = static_cast<u32>(write(frame->ebx, frame->ecx, frame->edx));
        return esp;
    case SyscallNumber::Sleep:
        frame->eax = static_cast<u32>(sleep(frame->ebx));
        return esp;
    case SyscallNumber::Uptime:
        frame->eax = static_cast<u32>(uptime());
        return esp;
    case SyscallNumber::Reboot:
        reboot();
        return esp;
    case SyscallNumber::Shutdown:
        shutdown();
        return esp;
    case SyscallNumber::Exit:
        exit(frame->ebx);
        return esp;
    case SyscallNumber::MapDevice:
        frame->eax = static_cast<u32>(mapDevice(frame->ebx, frame->ecx, frame->edx));
        return esp;
    case SyscallNumber::Notify:
        frame->eax = static_cast<u32>(notify(frame->ebx, (Message*)frame->ecx));
        return esp;
    case SyscallNumber::MemInfo: {
        u32 total, used, free;
        memInfo(total, used, free);
        frame->eax = total;
        frame->ebx = used;
        frame->ecx = free;
        return esp;
    }
    default:
        frame->eax = static_cast<u32>(-1);
        return esp;
    }
}

u32 handleSyscall(u32 esp) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    return sh.handleSyscall(esp);
}
