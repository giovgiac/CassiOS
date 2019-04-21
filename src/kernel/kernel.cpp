/**
 * kernel.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "kernel/kernel.hpp"

void outputs(const char* str) {
    unsigned short* buffer = reinterpret_cast<unsigned short*>(0xb8000);

    for (unsigned int i = 0; str[i] != '\0' ; ++i) {
        // Copy High-Bits and Merge with New Low-Bits
        buffer[i] = (buffer[i] & 0xFF00) | str[i];
    }
}

extern "C" void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

extern "C" void cassio(void* multiboot, unsigned int magic) {
    outputs("Hello World!");
    while (true);
}
