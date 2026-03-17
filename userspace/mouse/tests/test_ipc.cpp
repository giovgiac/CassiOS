/**
 * test_ipc.cpp -- mouse service IPC integration tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <test.hpp>
#include <ns.hpp>
#include <ipc.hpp>
#include <message.hpp>

using namespace cassio;

TEST(mouse_ipc_service_registered) {
    u32 pid = Nameserver::lookup("mouse");
    ASSERT(pid != 0);
}

TEST(mouse_ipc_read_state) {
    u32 pid = Nameserver::lookup("mouse");
    ASSERT(pid != 0);

    Message msg = {};
    msg.type = MessageType::MouseRead;
    i32 ret = IPC::send(pid, &msg);
    ASSERT_EQ(ret, 0);

    // No mouse movement in QEMU without input, so deltas should be zero.
    // Buttons should also be zero.
    ASSERT_EQ(msg.arg1, 0u);
}
