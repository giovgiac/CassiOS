/**
 * interrupt.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/interrupt.hpp"
#include "hardware/exception.hpp"
#include "hardware/irq.hpp"
#include "core/syscall.hpp"

using namespace cassio;
using namespace cassio::hardware;

InterruptManager InterruptManager::instance;

void InterruptManager::activate() {
    asm("sti");
}

void InterruptManager::deactivate() {
    asm("cli");
}

InterruptManager::InterruptManager() : code_offset(0) {}

void InterruptManager::setInterrupt(u8 number, u16 code_offset, void(*handler)(), u8 access, u8 flags) {
    idt[number].handler_low = ((u32)handler) & 0xFFFF;
    idt[number].handler_high = (((u32)handler) >> 16) & 0xFFFF;
    idt[number].code_offset = code_offset;
    idt[number].access = IDT_DESCRIPTOR_PRESENT | ((access & 3) << 5) | flags;
    idt[number].reserved = 0;
}

void InterruptManager::setInterruptGate(u8 vector, void(*handler)()) {
    setInterrupt(vector, code_offset, handler, 0, IDT_INTERRUPT_GATE);
}

void InterruptManager::setTrapGate(u8 vector, void(*handler)(), u8 dpl) {
    setInterrupt(vector, code_offset, handler, dpl, IDT_TRAP_GATE);
}

void InterruptManager::load(cassio::kernel::GlobalDescriptorTable& gdt) {
    code_offset = gdt.getCodeOffset();

    for (u16 i = 0; i < 256; ++i) {
        setInterrupt(i, code_offset, &ignoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
    }

    ExceptionHandler::getHandler().load();
    IrqManager::getManager().load();
    kernel::SyscallHandler::getSyscallHandler().load();

    InterruptDescriptorTable table;
    table.size = 256 * sizeof(GateDescriptor) - 1;
    table.base = (u32)idt;

    asm volatile("lidt  %0": :"m" (table));
}
