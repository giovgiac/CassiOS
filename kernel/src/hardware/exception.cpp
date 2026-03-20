/**
 * exception.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/exception.hpp"

#include "hardware/interrupt.hpp"
#include "hardware/serial.hpp"

#include <std/fmt.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::hardware;

ExceptionHandler ExceptionHandler::instance;

ExceptionHandler::ExceptionHandler() {}

void ExceptionHandler::load() {
    InterruptManager& im = InterruptManager::getManager();
    im.setInterruptGate(0x00, &handleException0x00);
    im.setInterruptGate(0x06, &handleException0x06);
    im.setInterruptGate(0x0D, &handleException0x0D);
    im.setInterruptGate(0x0E, &handleException0x0E);
}

u32 ExceptionHandler::handle(u8 vector, u32 error_code, u32 esp) {
    Serial& com1 = COM1::getSerial();
    char buf[48];
    fmt::format(buf, sizeof(buf), "EXCEPTION: vector=%u error_code=%u\n", (u32)vector, error_code);
    com1.puts(buf);

    while (true) {
        asm volatile("cli; hlt");
    }

    return esp;
}

u32 handleException(u8 vector, u32 error_code, u32 esp) {
    ExceptionHandler& eh = ExceptionHandler::getHandler();
    return eh.handle(vector, error_code, esp);
}
