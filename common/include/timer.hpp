/**
 * timer.hpp -- Shared timer constants
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_TIMER_HPP_
#define COMMON_TIMER_HPP_

#include <types.hpp>

namespace cassio {

// System tick frequency in Hz. Used by both kernel (PIT driver) and
// userspace (uptime conversion) to avoid hardcoded magic numbers.
constexpr u32 TICK_FREQUENCY = 1000;

} // cassio

#endif // COMMON_TIMER_HPP_
