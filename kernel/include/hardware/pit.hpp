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

namespace cassio {
namespace hardware {

class PitTimer {
public:
    inline static PitTimer& getTimer() { return instance; }

    /** Programs channel 0 to tick at TICK_FREQUENCY. */
    void activate() { pit_activate(); }

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
