/**
 * mouse.cpp -- mouse service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/mouse.hpp>
#include <std/ipc.hpp>
#include <std/ns.hpp>

using namespace std;

mouse::Mouse::Mouse() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("mouse");
    }
}

void mouse::Mouse::read(u8& buttons, i32& dx, i32& dy) {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::MouseRead;
    ipc::send(pid, &msg);
    buttons = static_cast<u8>(msg.arg1);
    dx = static_cast<i32>(msg.arg2);
    dy = static_cast<i32>(msg.arg3);
}
