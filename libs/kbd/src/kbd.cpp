/**
 * kbd.cpp -- keyboard service client implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/ipc.hpp>
#include <std/kbd.hpp>
#include <std/ns.hpp>

using namespace std;

kbd::Kbd::Kbd() : pid(0) {
    while (pid == 0) {
        pid = ns::lookup("kbd");
    }
}

u8 kbd::Kbd::read() {
    ipc::Message msg = {};
    msg.type = ipc::MessageType::KbdRead;
    ipc::send(pid, &msg);
    return static_cast<u8>(msg.arg1);
}
