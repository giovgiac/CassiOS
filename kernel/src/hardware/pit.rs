// pit.rs -- Programmable Interval Timer
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Channel 0 ticks TICK_FREQUENCY times a second on IRQ 0; each tick counts
// and gives the scheduler a chance to switch process. The C++ PitTimer
// (kernel/include/hardware/pit.hpp) forwards here until its callers are
// ported.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

use sys::os::TICK_FREQUENCY;

/// The PIT's input clock in Hz.
const BASE_FREQUENCY: u32 = 1_193_182;
/// Channel 0 counts down from this, once per tick.
pub const DIVISOR: u16 = (BASE_FREQUENCY / TICK_FREQUENCY) as u16;
/// Channel 0, low then high byte, rate generator (mode 2).
const CHANNEL0_RATE_GENERATOR: u8 = 0x34;

/// Ticks to wait for `ms` milliseconds, capped at i32::MAX ticks (about
/// 24.8 days): the tick counter wraps, so a wait is measured as a signed
/// distance to its target.
pub fn ticks_for(ms: u32) -> u32 {
    (ms as u64 * TICK_FREQUENCY as u64 / 1000).min(i32::MAX as u64) as u32
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;
    use core::sync::atomic::{AtomicU32, Ordering};

    use sys::io::{Port, PortType};

    use super::*;

    static TICKS: AtomicU32 = AtomicU32::new(0);

    /// Start ticking.
    pub fn activate() {
        let command = Port::<u8>::new(PortType::PitCommand);
        let channel0 = Port::<u8>::new(PortType::PitChannel0Data);
        // SAFETY: the kernel owns the PIT.
        unsafe {
            command.write(CHANNEL0_RATE_GENERATOR);
            channel0.write(DIVISOR as u8);
            channel0.write((DIVISOR >> 8) as u8);
        }
    }

    pub fn ticks() -> u32 {
        TICKS.load(Ordering::Relaxed)
    }

    /// Count a tick and let the scheduler switch process. Returns the
    /// frame to resume.
    pub fn tick(esp: u32) -> u32 {
        TICKS.fetch_add(1, Ordering::Relaxed);
        crate::system::scheduler::tick(esp)
    }

    /// Wait at least `ms` milliseconds. Interrupts must be enabled.
    pub fn sleep(ms: u32) {
        let target = ticks().wrapping_add(ticks_for(ms));
        while (target.wrapping_sub(ticks()) as i32) > 0 {
            unsafe { asm!("hlt", options(nomem, nostack)) };
        }
    }

    #[unsafe(no_mangle)]
    extern "C" fn pit_activate() {
        activate();
    }

    #[cfg(not(test))]
    pub mod ktests {
        use super::*;
        use crate::ktest::Outcome;

        tests!(pit_tick_counts, pit_sleep_zero_returns_at_once);

        fn pit_tick_counts() -> Outcome {
            let before = ticks();
            tick(0);
            check_eq!(ticks(), before + 1);
            Ok(())
        }

        fn pit_sleep_zero_returns_at_once() -> Outcome {
            // Interrupts are off in the test kernel, so any wait would hang.
            let before = ticks();
            sleep(0);
            check_eq!(ticks(), before);
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_divisor_gives_the_tick_frequency() {
        assert_eq!(DIVISOR, 1193);
        assert_eq!(BASE_FREQUENCY / DIVISOR as u32, 1000);
    }

    #[test]
    fn ticks_for_converts_milliseconds() {
        assert_eq!(ticks_for(0), 0);
        assert_eq!(ticks_for(20), 20 * TICK_FREQUENCY / 1000);
        assert_eq!(ticks_for(u32::MAX), i32::MAX as u32);
        // The longest wait still compares as in the future.
        assert!((ticks_for(u32::MAX) as i32) > 0);
    }
}
