/**
 * test_ipc.cpp -- keyboard service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/test.hpp>
#include <ns.hpp>

using namespace cassio;
using namespace std;

TEST(kbd_ipc_service_registered) {
    u32 kbd_pid = Nameserver::lookup("kbd");
    ASSERT(kbd_pid != 0);
}
