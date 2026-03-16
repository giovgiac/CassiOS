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
#include "drivers/pit.hpp"
#include "hardware/interrupt.hpp"
#include "hardware/port.hpp"
#include "hardware/serial.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::drivers;
using namespace cassio::hardware;

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
        copyMessage(&sender->msg, targetBuf);

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

    // No pending senders -- block.
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

    // Copy reply to sender's reply buffer.
    Message* senderBuf = (Message*)target->msgPtr;
    copyMessage(msg, senderBuf);

    // Set sender's return value to 0 (success).
    SyscallFrame* targetFrame = (SyscallFrame*)target->esp;
    targetFrame->eax = 0;

    target->state = ProcessState::Ready;
    return 0;
}

i32 SyscallHandler::write(u32 fd, u32 buf, u32 len) {
    const char* str = reinterpret_cast<const char*>(buf);

    if (fd == 1) {
        VgaTerminal& vga = VgaTerminal::getTerminal();
        for (u32 i = 0; i < len; ++i) {
            vga.putchar(str[i]);
        }
        return static_cast<i32>(len);
    }

    if (fd == 2) {
        Serial& serial = COM1::getSerial();
        for (u32 i = 0; i < len; ++i) {
            serial.putchar(str[i]);
        }
        return static_cast<i32>(len);
    }

    return -1;
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
        frame->eax = static_cast<u32>(result);
        return esp;
    }
    case SyscallNumber::Reply: {
        i32 result = reply(frame->ebx, (Message*)frame->ecx);
        frame->eax = static_cast<u32>(result);
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
    default:
        frame->eax = static_cast<u32>(-1);
        return esp;
    }
}

u32 handleSyscall(u32 esp) {
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    return sh.handleSyscall(esp);
}
