/**
 * test_ipc.cpp -- ATA service IPC integration tests
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

TEST(ata_ipc_service_registered) {
    u32 pid = Nameserver::lookup("ata");
    ASSERT(pid != 0);
}

TEST(ata_ipc_read_replies) {
    u32 pid = Nameserver::lookup("ata");
    ASSERT(pid != 0);

    Message msg = {};
    msg.type = MessageType::AtaRead;
    msg.arg1 = 0;  // LBA 0
    msg.arg2 = 0;  // byte offset 0
    i32 ret = IPC::send(pid, &msg);
    ASSERT_EQ(ret, 0);

    // With a disk present, should read 16 bytes.
    ASSERT_EQ(msg.arg1, 16u);
}
