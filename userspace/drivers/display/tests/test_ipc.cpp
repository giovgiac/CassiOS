/**
 * test_ipc.cpp -- Display service IPC tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ipc.hpp>
#include <std/ns.hpp>
#include <std/test.hpp>

using namespace std;

TEST(display_ipc_service_registered) {
    u32 pid = ns::lookup("display");
    ASSERT(pid > 0);
}

TEST(display_ipc_get_info_responds) {
    u32 pid = ns::lookup("display");
    ASSERT(pid > 0);

    ipc::Message msg = {};
    msg.type = ipc::MessageType::DisplayGetInfo;
    i32 result = ipc::send(pid, &msg);

    // Service should respond without error. Actual dimensions depend on
    // GRUB framebuffer setup which isn't available in the test environment.
    ASSERT_EQ(result, 0);
}
