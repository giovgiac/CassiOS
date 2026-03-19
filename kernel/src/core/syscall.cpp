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
#include <std/mem.hpp>
#include "core/scheduler.hpp"
#include "hardware/pit.hpp"
#include "hardware/interrupt.hpp"
#include "hardware/irq.hpp"
#include <std/io.hpp>
#include "hardware/serial.hpp"
#include "memory/paging.hpp"
#include "memory/physical.hpp"

using namespace cassio;
using namespace std;
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
    mem::copy(dst, src, sizeof(Message));
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

// Copy bulk data from the current address space to a target process's address
// space. Uses a 256-byte kernel stack buffer for chunked cross-PD transfer.
static void copyDataToProcess(Process* target, u8* dst, const u8* src, u32 len) {
    u32 currentCR3;
    asm volatile("mov %%cr3, %0" : "=r"(currentCR3));

    u32 targetPD = target->pageDirectory;
    if (targetPD != 0 && targetPD != currentCR3) {
        u8 chunk[256];
        u32 offset = 0;
        while (offset < len) {
            u32 n = len - offset;
            if (n > 256) n = 256;

            mem::copy(chunk, src + offset, n);

            asm volatile("mov %0, %%cr3" : : "r"(targetPD) : "memory");
            mem::copy(dst + offset, chunk, n);
            asm volatile("mov %0, %%cr3" : : "r"(currentCR3) : "memory");

            offset += n;
        }
    } else {
        mem::copy(dst, src, len);
    }
}

// Copy bulk data from a source process's address space to the current address
// space. Uses a 256-byte kernel stack buffer for chunked cross-PD transfer.
static void copyDataFromProcess(Process* source, u8* dst, const u8* src, u32 len) {
    u32 currentCR3;
    asm volatile("mov %%cr3, %0" : "=r"(currentCR3));

    u32 sourcePD = source->pageDirectory;
    if (sourcePD != 0 && sourcePD != currentCR3) {
        u8 chunk[256];
        u32 offset = 0;
        while (offset < len) {
            u32 n = len - offset;
            if (n > 256) n = 256;

            asm volatile("mov %0, %%cr3" : : "r"(sourcePD) : "memory");
            mem::copy(chunk, src + offset, n);
            asm volatile("mov %0, %%cr3" : : "r"(currentCR3) : "memory");

            mem::copy(dst + offset, chunk, n);

            offset += n;
        }
    } else {
        mem::copy(dst, src, len);
    }
}

i32 SyscallHandler::send(u32 targetPid, Message* msg, u32 dataPtr, u32 dataLen) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* sender = pm.current();
    Process* target = pm.get(targetPid);

    if (!target || target == sender) {
        return -1;
    }

    // Copy outgoing message into sender's process struct.
    copyMessage(msg, &sender->msg);
    sender->msgPtr = (u32)msg;  // Reply will be written here.
    sender->dataPtr = dataPtr;
    sender->dataLen = dataLen;

    if (target->state == ProcessState::ReceiveBlocked) {
        // Target is waiting for a message -- deliver immediately.
        Message* targetBuf = (Message*)target->msgPtr;
        copyMessageToProcess(target, targetBuf, &sender->msg);

        // Copy bulk data if both sides provided buffers.
        if (dataPtr != 0 && dataLen > 0 &&
            target->dataPtr != 0 && target->dataLen > 0) {
            u32 copyLen = dataLen < target->dataLen ? dataLen : target->dataLen;
            copyDataToProcess(target, (u8*)target->dataPtr,
                              (const u8*)dataPtr, copyLen);
        }

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

i32 SyscallHandler::receive(Message* msg, u32 dataPtr, u32 dataCapacity) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* receiver = pm.current();

    // Save data buffer info (used if we block or deliver from queue).
    receiver->dataPtr = dataPtr;
    receiver->dataLen = dataCapacity;

    // Check for pending IRQ notifications first (no bulk data).
    IrqManager& irqMgr = IrqManager::getManager();
    if (irqMgr.deliverPending(receiver->pid, msg)) {
        return -2;  // IRQ notification delivered; handleSyscall sets eax=0.
    }

    // Drain queued notifications before blocked senders so that
    // fire-and-forget messages are processed in chronological order
    // relative to blocking sends from the same source.
    u32 notifySender;
    if (receiver->notifyPop(notifySender, *msg,
                            (void*)dataPtr, dataCapacity)) {
        return -3;
    }

    if (!receiver->sendQueue.isEmpty()) {
        // Pending sender -- deliver immediately.
        u32 senderPid = receiver->sendQueuePop();
        Process* sender = pm.get(senderPid);
        if (!sender) {
            return -1;
        }

        copyMessage(&sender->msg, msg);

        // Copy bulk data from sender to receiver.
        if (sender->dataPtr != 0 && sender->dataLen > 0 &&
            dataPtr != 0 && dataCapacity > 0) {
            u32 copyLen = sender->dataLen < dataCapacity
                        ? sender->dataLen : dataCapacity;
            copyDataFromProcess(sender, (u8*)dataPtr,
                                (const u8*)sender->dataPtr, copyLen);
        }

        return static_cast<i32>(senderPid);
    }

    // No pending IRQs, notifications, or senders -- block.
    receiver->msgPtr = (u32)msg;
    receiver->state = ProcessState::ReceiveBlocked;
    return 0;  // Caller should block.
}

i32 SyscallHandler::reply(u32 targetPid, Message* msg, u32 dataPtr, u32 dataLen) {
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

    // Copy reply bulk data to sender's data buffer.
    if (dataPtr != 0 && dataLen > 0 &&
        target->dataPtr != 0 && target->dataLen > 0) {
        u32 copyLen = dataLen < target->dataLen ? dataLen : target->dataLen;
        copyDataToProcess(target, (u8*)target->dataPtr,
                          (const u8*)dataPtr, copyLen);
    }

    // Set sender's return value to 0 (success).
    SyscallFrame* targetFrame = (SyscallFrame*)target->esp;
    targetFrame->eax = 0;

    target->state = ProcessState::Ready;
    return 0;
}

i32 SyscallHandler::notify(u32 targetPid, Message* msg, u32 dataPtr, u32 dataLen) {
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

        // Copy bulk data if both sides provided buffers.
        if (dataPtr != 0 && dataLen > 0 &&
            target->dataPtr != 0 && target->dataLen > 0) {
            u32 copyLen = dataLen < target->dataLen ? dataLen : target->dataLen;
            copyDataToProcess(target, (u8*)target->dataPtr,
                              (const u8*)dataPtr, copyLen);
        }

        // Return 0 as sender so receiver knows not to reply.
        SyscallFrame* targetFrame = (SyscallFrame*)target->esp;
        targetFrame->eax = 0;

        target->state = ProcessState::Ready;
    } else {
        // Queue for later delivery. Copy data to kernel heap since the
        // sender's address space won't be available when it's delivered.
        if (!target->notifyPush(sender->pid, temp,
                                (const void*)dataPtr, dataLen)) {
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

    // Only allow mapping physical addresses below 1MB (device/MMIO region).
    // The sub-1MB region is at most 256 pages; reject larger counts to
    // prevent pages * 0x1000 from wrapping u32.
    if (pages > 0x100) {
        return -1;
    }
    u32 endPhysical = physical + pages * 0x1000;
    if (endPhysical < physical || endPhysical > 0x100000) {
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
    Port<u8> kbCmd(PortType::KbdCommand);
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

u32 SyscallHandler::procList(ProcEntry* buf, u32 maxEntries) {
    ProcessManager& pm = ProcessManager::getManager();
    u32 count = 0;

    for (Process* p = pm.getHead(); p && count < maxEntries; p = p->next) {
        if (p->pid == 0) {
            continue;  // Skip kernel task.
        }
        buf[count].pid = p->pid;
        buf[count].state = static_cast<u32>(p->state);
        buf[count].heapSize = (p->heapBreak > p->heapBase)
                            ? (p->heapBreak - p->heapBase) : 0;
        ++count;
    }

    return count;
}

u32 SyscallHandler::sbrk(u32 increment) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* caller = pm.current();

    if (!caller || caller->heapBreak == 0) {
        return 0;
    }

    u32 oldBreak = caller->heapBreak;

    if (increment == 0) {
        return oldBreak;
    }

    // Check for u32 overflow.
    if (oldBreak + increment < oldBreak) {
        return 0;
    }

    u32 newBreak = (oldBreak + increment + memory::FRAME_SIZE - 1) & ~(memory::FRAME_SIZE - 1);

    // The page-alignment rounding can itself wrap when oldBreak + increment
    // falls near 0xFFFFFFFF.
    if (newBreak < oldBreak) {
        return 0;
    }

    // Prevent heap from colliding with the user stack (top page at 0xBFFFF000).
    static constexpr u32 USER_STACK_BOTTOM = 0xBFFFF000;
    if (newBreak > USER_STACK_BOTTOM) {
        return 0;
    }

    memory::PagingManager& paging = memory::PagingManager::getManager();
    memory::PhysicalMemoryManager& pmm = memory::PhysicalMemoryManager::getManager();

    for (u32 addr = oldBreak; addr < newBreak; addr += memory::FRAME_SIZE) {
        void* frame = pmm.allocFrame();
        if (!frame) {
            return 0;
        }
        paging.mapUserPage(caller->pageDirectory, addr, (u32)frame,
                           memory::PAGE_PRESENT | memory::PAGE_READWRITE | memory::PAGE_USER);

        // Flush TLB for the newly mapped page so userspace doesn't fault
        // on first access due to stale TLB entries.
        paging.flushTLB(addr);
    }

    caller->heapBreak = newBreak;
    return oldBreak;
}

u32 SyscallHandler::handleSyscall(u32 esp) {
    SyscallFrame* frame = (SyscallFrame*)esp;
    u32 number = frame->eax;

    switch (number) {
    case SyscallNumber::Send: {
        i32 result = send(frame->ebx, (Message*)frame->ecx,
                          frame->esi, frame->edi);
        if (result == 0) {
            Scheduler& sched = Scheduler::getScheduler();
            return sched.reschedule(esp);
        }
        frame->eax = static_cast<u32>(result);
        return esp;
    }
    case SyscallNumber::Receive: {
        i32 result = receive((Message*)frame->ebx,
                             frame->esi, frame->edi);
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
        i32 result = reply(frame->ebx, (Message*)frame->ecx,
                           frame->esi, frame->edi);
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
        frame->eax = static_cast<u32>(notify(frame->ebx, (Message*)frame->ecx,
                                             frame->esi, frame->edi));
        return esp;
    case SyscallNumber::MemInfo: {
        u32 total, used, free;
        memInfo(total, used, free);
        frame->eax = total;
        frame->ebx = used;
        frame->ecx = free;
        return esp;
    }
    case SyscallNumber::Sbrk:
        frame->eax = sbrk(frame->ebx);
        return esp;
    case SyscallNumber::ProcList:
        frame->eax = procList((ProcEntry*)frame->ebx, frame->ecx);
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
