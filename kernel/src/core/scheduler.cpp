/**
 * scheduler.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/scheduler.hpp"

using namespace cassio;
using namespace cassio::kernel;

Scheduler Scheduler::instance;

Scheduler::Scheduler()
    : gdt(nullptr), tickCount(0), timeSlice(DEFAULT_TIME_SLICE) {}

void Scheduler::init(GlobalDescriptorTable& gdt) {
    this->gdt = &gdt;
}

u32 Scheduler::switchTo(Process* next, Process* current, u32 currentEsp) {
    current->esp = currentEsp;

    next->state = ProcessState::Running;

    // Load new page directory if it differs from the current one.
    if (next->pageDirectory != current->pageDirectory && next->pageDirectory != 0) {
        asm volatile("mov %0, %%cr3" : : "r"(next->pageDirectory));
    }

    // Update TSS.esp0 for ring 3 -> ring 0 transitions.
    if (gdt && next->kernelEsp) {
        gdt->setTssEsp0(next->kernelEsp);
    }

    // Keep ProcessManager in sync.
    ProcessManager::getManager().setCurrent(next);

    return next->esp;
}

Process* Scheduler::findNextReady(Process* current) {
    ProcessManager& pm = ProcessManager::getManager();

    // Start at the next process in the list, wrapping to head.
    Process* candidate = current->next;
    if (!candidate) {
        candidate = pm.getHead();
    }

    while (candidate != current) {
        if (candidate->state == ProcessState::Ready) {
            return candidate;
        }
        candidate = candidate->next;
        if (!candidate) {
            candidate = pm.getHead();
        }
    }

    // No other Ready process found; stay on current.
    return current;
}

u32 Scheduler::schedule(u32 currentEsp) {
    ProcessManager& pm = ProcessManager::getManager();
    if (pm.getProcessCount() <= 1) {
        return currentEsp;
    }

    tickCount++;
    if (tickCount < timeSlice) {
        return currentEsp;
    }
    tickCount = 0;

    Process* current = pm.current();

    // Only demote Running -> Ready. Leave SendBlocked/ReceiveBlocked alone.
    if (current->state == ProcessState::Running) {
        current->state = ProcessState::Ready;
    }

    Process* next = findNextReady(current);
    return switchTo(next, current, currentEsp);
}

u32 Scheduler::reschedule(u32 currentEsp) {
    ProcessManager& pm = ProcessManager::getManager();
    if (pm.getProcessCount() <= 1) {
        return currentEsp;
    }

    Process* current = pm.current();
    // State already set by caller (SendBlocked/ReceiveBlocked).

    Process* next = findNextReady(current);
    tickCount = 0;
    return switchTo(next, current, currentEsp);
}

void Scheduler::reset() {
    tickCount = 0;
}
