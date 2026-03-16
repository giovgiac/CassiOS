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

    return esp;
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
