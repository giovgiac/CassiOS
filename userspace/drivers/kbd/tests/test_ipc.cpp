/**
 * test_ipc.cpp -- keyboard service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ns.hpp>
#include <std/test.hpp>

using namespace std;

TEST(kbd_ipc_service_registered) {
    u32 kbd_pid = ns::lookup("kbd");
    ASSERT(kbd_pid != 0);
}
