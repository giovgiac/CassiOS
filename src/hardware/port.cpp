/**
 * port.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/port.hpp"

using namespace cassio;
using namespace cassio::hardware;

/** 8-bit Port Methods */

Port<u8>::Port(PortType type)
    : number(static_cast<u16>(type)) {}

u8 Port<u8>::read() {
    u8 result;
    asm volatile("inb   %1, %0": "=a" (result): "Nd" (number));
    return result;
}

void Port<u8>::write(u8 data) {
    asm volatile("outb  %0, %1": :"a" (data), "Nd" (number));
}

void Port<u8>::writeSlow(u8 data) {
    asm volatile("outb  %0, %1\njmp 1f\n1: jmp 1f\n1:": :"a" (data), "Nd" (number));
}

/** 16-bit Port Methods */

Port<u16>::Port(PortType type)
    : number(static_cast<u16>(type)) {}

u16 Port<u16>::read() {
    u16 result;
    asm volatile("inw   %1, %0": "=a" (result): "Nd" (number));
    return result;
}

void Port<u16>::write(u16 data) {
    asm volatile("outw  %0, %1": :"a" (data), "Nd" (number));
}

/** 32-bit Port Methods */

Port<u32>::Port(PortType type)
    : number(static_cast<u16>(type)) {}

u32 Port<u32>::read() {
    u32 result;
    asm volatile("inl   %1, %0": "=a" (result): "Nd" (number));
    return result;
}

void Port<u32>::write(u32 data) {
    asm volatile("outl  %0, %1": :"a" (data), "Nd" (number));
}

