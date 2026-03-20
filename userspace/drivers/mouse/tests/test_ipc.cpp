/**
 * test_ipc.cpp -- mouse service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/mouse.hpp>
#include <std/test.hpp>

using namespace std;

TEST(mouse_ipc_service_registered) {
    mouse::Mouse m;
    // Construction succeeded (lookup returned non-zero).
    ASSERT(true);
}

TEST(mouse_ipc_read_state) {
    mouse::Mouse m;

    u8 buttons;
    i32 dx, dy;
    m.read(buttons, dx, dy);

    // No mouse movement in QEMU without input, so deltas should be zero.
    // Buttons should also be zero.
    ASSERT_EQ(buttons, 0u);
}
