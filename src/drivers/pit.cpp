/**
 * pit.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "drivers/pit.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::hardware;

PitTimer PitTimer::instance;

PitTimer::PitTimer()
    : Driver(DriverType::SystemTimer),
      channel0(PortType::PitChannel0Data),
      command(PortType::PitCommand),
      ticks(0) {}

void PitTimer::deactivate() {}

void PitTimer::activate() {
    command.write(PIT_CMD_CHANNEL0_MODE2);
    channel0.write(static_cast<u8>(PIT_DIVISOR & 0xFF));
    channel0.write(static_cast<u8>((PIT_DIVISOR >> 8) & 0xFF));
}

u32 PitTimer::handleInterrupt(u32 esp) {
    ticks += 1;
    return esp;
}

u32 PitTimer::getTicks() {
    return ticks;
}

void PitTimer::sleep(u32 ms) {
    u32 target = ticks + (ms * PIT_FREQUENCY) / 1000;
    while (ticks < target) {
        asm volatile("hlt");
    }
}
