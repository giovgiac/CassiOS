/**
 * interrupt.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/interrupt.hpp"

using namespace cassio::hardware;

InterruptManager InterruptManager::instance;

void outputs(const char* str);

void InterruptManager::activate() {
    asm("sti");
}

void InterruptManager::deactivate() {
    asm("cli");
}

InterruptManager::InterruptManager()
    : pic_master_cmd(0x20), pic_master_data(0x21), pic_slave_cmd(0xA0), pic_slave_data(0xA1) {}

void InterruptManager::setInterrupt(u8 number, u16 code_offset, void(*handler)(), u8 access, u8 flags) {
    idt[number].handler_low = ((u32)handler) & 0xFFFF;
    idt[number].handler_high = (((u32)handler) >> 16) & 0xFFFF;
    idt[number].code_offset = code_offset;
    idt[number].access = IDT_DESCRIPTOR_PRESENT | ((access & 3) << 5) | flags;
    idt[number].reserved = 0;
}

void InterruptManager::load(cassio::kernel::GlobalDescriptorTable& gdt) {
    u16 code_offset = gdt.getCodeOffset();

    for (u16 i = 0; i < 256; ++i) {
        drv[i] = nullptr;
        setInterrupt(i, code_offset, &ignoreInterruptRequest, 0, IDT_INTERRUPT_GATE);
    }

    setInterrupt(0x20, code_offset, &handleInterruptRequest0x00, 0, IDT_INTERRUPT_GATE);
    setInterrupt(0x21, code_offset, &handleInterruptRequest0x01, 0, IDT_INTERRUPT_GATE);

    /** Tells Master PIC to Add 20 to Interrupts and Slave PIC to Add 28 */
    pic_master_cmd.writeSlow(0x11);
    pic_slave_cmd.writeSlow(0x11);

    pic_master_data.writeSlow(0x20);
    pic_slave_data.writeSlow(0x28);

    /** Configures PIC */
    pic_master_data.writeSlow(0x04);
    pic_slave_data.writeSlow(0x02);

    pic_master_data.writeSlow(0x01);
    pic_slave_data.writeSlow(0x01);

    pic_master_data.writeSlow(0x00);
    pic_slave_data.writeSlow(0x00);

    InterruptDescriptorTable table;
    table.size = 256 * sizeof(GateDescriptor) - 1;
    table.base = (u32)idt;

    asm volatile("lidt  %0": :"m" (table));
}

void InterruptManager::unload() {

}

u32 InterruptManager::handleInterrupt(u8 number, u32 esp) {
    if (drv[number] != nullptr) {
        esp = drv[number]->handleInterrupt(esp);
    }
    else if (number != 0x20) {
        // Print if the interrupt is not a 'timer interrupt' <- interrupt number 0x20.
        outputs("Unhandled Interrupt Triggered!\n");
    }

    if (0x20 <= number && number < 0x30) {
        // Tell the PIC that interrupt was handled.
        pic_master_cmd.writeSlow(0x20);

        if (0x28 <= number) {
            // If interrupt came from slave, tell the slave that interrupt was handled.
            pic_slave_cmd.writeSlow(0x20);
        }
    }

    return esp;
}

u32 handleInterrupt(u8 number, u32 esp) {
    InterruptManager& im = InterruptManager::getManager();
    // TODO: Check if is loaded (add boolean to keep loaded as status)
    return im.handleInterrupt(number, esp);
}
