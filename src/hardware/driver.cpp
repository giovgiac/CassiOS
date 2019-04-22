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

Driver::Driver(u8 num) : number(num) {
    InterruptManager& im = InterruptManager::getManager();
    im.drv[num] = this;
}

Driver::~Driver() {
    InterruptManager& im = InterruptManager::getManager();

    if (im.drv[number] == this) {
        im.drv[number] = nullptr;
    }
}

u32 Driver::handleInterrupt(u32 esp) {
    return esp;
}