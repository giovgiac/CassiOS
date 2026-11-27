/**
 * interrupt.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/interrupt.hpp"

#include "core/syscall.hpp"

using namespace cassio;
using namespace cassio::hardware;

InterruptManager InterruptManager::instance;

void InterruptManager::load(cassio::kernel::GlobalDescriptorTable&) {
    interrupt_init();
    kernel::SyscallHandler::getSyscallHandler().load();
}
