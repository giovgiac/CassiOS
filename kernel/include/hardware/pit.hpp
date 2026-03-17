/**
 * pit.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_PIT_HPP_
#define HARDWARE_PIT_HPP_

#include <types.hpp>
#include <hardware/irq.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

// PIT oscillator base frequency in Hz.
constexpr u32 PIT_BASE_FREQUENCY = 1193182;

// Target tick frequency in Hz.
constexpr u32 PIT_FREQUENCY = 100;

// Divisor to achieve the target frequency.
constexpr u16 PIT_DIVISOR = PIT_BASE_FREQUENCY / PIT_FREQUENCY;

// PIT command byte: channel 0, lo/hi byte, rate generator (mode 2).
constexpr u8 PIT_CMD_CHANNEL0_MODE2 = 0x34;

/**
 * @brief PIT timer driver singleton.
 *
 * Programs the 8253/8254 PIT channel 0 for periodic ticks at ~100 Hz.
 * Maintains a tick counter and provides a busy-wait sleep.
 *
 */
class PitTimer {
private:
    hardware::Port<u8> channel0;
    hardware::Port<u8> command;
    volatile u32 ticks;

    static PitTimer instance;

private:
    PitTimer();
    ~PitTimer() = default;

public:
    /**
     * @brief Returns the PitTimer singleton instance.
     *
     */
    inline static PitTimer& getTimer() {
        return instance;
    }

    /**
     * @brief Static IRQ handler registered with IrqManager.
     *
     */
    static u32 irqHandler(u32 esp);

    /**
     * @brief Programs the PIT for periodic mode at ~100 Hz.
     *
     */
    void activate();

    /**
     * @brief Increments the tick counter and invokes the scheduler.
     *
     */
    u32 handleInterrupt(u32 esp);

    /**
     * @brief Returns the number of ticks since activation.
     *
     */
    u32 getTicks();

    /**
     * @brief Busy-waits for the given number of milliseconds.
     *
     */
    void sleep(u32 ms);

    /** Deleted Methods */
    PitTimer(const PitTimer&) = delete;
    PitTimer(PitTimer&&) = delete;
    PitTimer& operator=(const PitTimer&) = delete;
    PitTimer& operator=(PitTimer&&) = delete;
};

} // hardware
} // cassio

#endif // HARDWARE_PIT_HPP_
