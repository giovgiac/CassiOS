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

using namespace cassio;
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
    com1.puts("EXCEPTION: vector=");
    com1.put_dec(vector);
    com1.puts(" error_code=");
    com1.put_dec(error_code);
    com1.putchar('\n');

    while (true) {
        asm volatile("cli; hlt");
    }

    return esp;
}

u32 handleException(u8 vector, u32 error_code, u32 esp) {
    ExceptionHandler& eh = ExceptionHandler::getHandler();
    return eh.handle(vector, error_code, esp);
}
