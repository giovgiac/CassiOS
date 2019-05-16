/**
 * driver.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/driver.hpp"

#include <hardware/interrupt.hpp>

using namespace cassio::hardware;

DriverManager DriverManager::instance;

Driver::Driver(DriverType type) : number(static_cast<u8>(type)) {
    InterruptManager& im = InterruptManager::getManager();
    im.drv[number] = this;
}

Driver::~Driver() {
    InterruptManager& im = InterruptManager::getManager();

    if (im.drv[number] == this) {
        im.drv[number] = nullptr;
    }
}

void Driver::activate() {

}

void Driver::deactivate() {

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
