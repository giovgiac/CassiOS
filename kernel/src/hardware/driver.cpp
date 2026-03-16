/**
 * driver.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/driver.hpp"

using namespace cassio;
using namespace cassio::hardware;

DriverManager DriverManager::instance;

Driver::Driver(DriverType type) : number(static_cast<u8>(type)) {
    IrqManager& irq = IrqManager::getManager();
    irq.registerDriver(number, this);
}

Driver::~Driver() {
    IrqManager& irq = IrqManager::getManager();
    irq.unregisterDriver(number, this);
}

i32 Driver::reset() {
    return 0;
}

u32 Driver::handleInterrupt(u32 esp) {
    return esp;
}

DriverManager::DriverManager() : size(0) {
    for (u32 i = 0; i < NUM_DRIVERS; ++i) {
        drivers[i] = nullptr;
    }
}

void DriverManager::addDriver(Driver& drv) {
    drivers[size++] = &drv;
}

void DriverManager::load() {
    for (u32 i = 0; i < size; ++i) {
        drivers[i]->activate();
    }
}

void DriverManager::unload() {
    for (u32 i = 0; i < size; ++i) {
        drivers[i]->deactivate();
    }
}
