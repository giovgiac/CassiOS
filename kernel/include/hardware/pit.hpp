/**
 * pit.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The PIT is Rust (kernel/src/hardware/pit.rs). PitTimer forwards to it
 * until its callers are ported.
 *
 */

#ifndef HARDWARE_PIT_HPP_
#define HARDWARE_PIT_HPP_

#include <std/types.hpp>

extern "C" void pit_activate();
extern "C" std::u32 pit_ticks();
extern "C" std::u32 pit_tick(std::u32 esp);
extern "C" void pit_sleep(std::u32 ms);

namespace cassio {
namespace hardware {

class PitTimer {
public:
    inline static PitTimer& getTimer() { return instance; }

    /** Programs channel 0 to tick at TICK_FREQUENCY. */
    void activate() { pit_activate(); }

    /** Counts a tick and lets the scheduler switch process. */
    std::u32 handleInterrupt(std::u32 esp) { return pit_tick(esp); }

    /** Ticks since activation. */
    std::u32 getTicks() { return pit_ticks(); }

    /** Waits at least ms milliseconds; interrupts must be enabled. */
    void sleep(std::u32 ms) { pit_sleep(ms); }

    /** Deleted Methods */
    PitTimer(const PitTimer&) = delete;
    PitTimer(PitTimer&&) = delete;
    PitTimer& operator=(const PitTimer&) = delete;
    PitTimer& operator=(PitTimer&&) = delete;

private:
    PitTimer() = default;

    static PitTimer instance;
};

} // namespace hardware
} // namespace cassio

#endif // HARDWARE_PIT_HPP_
