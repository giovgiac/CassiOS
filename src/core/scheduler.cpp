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
    : gdt(nullptr), tickCount(0), timeSlice(DEFAULT_TIME_SLICE),
      numProcesses(0), currentIndex(0) {
    for (u32 i = 0; i < ProcessManager::MAX_PROCESSES; i++) {
        processes[i] = nullptr;
    }
}

void Scheduler::init(GlobalDescriptorTable& gdt) {
    this->gdt = &gdt;
}

void Scheduler::addProcess(Process* process) {
    if (numProcesses < ProcessManager::MAX_PROCESSES) {
        processes[numProcesses++] = process;
    }
}

u32 Scheduler::schedule(u32 currentEsp) {
    if (numProcesses <= 1) {
        return currentEsp;
    }

    tickCount++;
    if (tickCount < timeSlice) {
        return currentEsp;
    }
    tickCount = 0;

    // Save current state.
    Process* current = processes[currentIndex];
    current->esp = currentEsp;
    current->state = ProcessState::Ready;

    // Find next Ready process (round-robin).
    u32 nextIndex = currentIndex;
    for (u32 i = 0; i < numProcesses; i++) {
        nextIndex = (nextIndex + 1) % numProcesses;
        if (processes[nextIndex]->state == ProcessState::Ready) {
            break;
        }
    }

    Process* next = processes[nextIndex];
    next->state = ProcessState::Running;
    currentIndex = nextIndex;

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

void Scheduler::reset() {
    tickCount = 0;
    numProcesses = 0;
    currentIndex = 0;
    for (u32 i = 0; i < ProcessManager::MAX_PROCESSES; i++) {
        processes[i] = nullptr;
    }
}
