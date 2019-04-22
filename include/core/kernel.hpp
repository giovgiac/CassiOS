/**
 * kernel.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_KERNEL_HPP_
#define CORE_KERNEL_HPP_

#include <core/gdt.hpp>
#include <core/types.hpp>
#include <drivers/keyboard.hpp>
#include <hardware/interrupt.hpp>
#include <hardware/port.hpp>

const u8 TERMINAL_WIDTH     = 80;
const u8 TERMINAL_HEIGHT    = 25;

/**
 * @brief
 *
 */
typedef void (*ctor)();

extern "C" ctor start_ctors;
extern "C" ctor end_ctors;

/**
 * @brief A function that outputs characters at the 80 by 25 terminal available at address 0xb8000.
 *
 * Each entry in the terminal is 2 bytes, with the first byte containing color information and the
 * second byte containing the character. Therefore, the function will maintain the color information
 * by ANDing with 0xFF00 and adding the new text information with an OR.
 *
 * @param str The string to output at the terminal.
 *
 */
void outputs(const char* str);

/**
 * @brief
 *
 */
extern "C" void ctors();

/**
 * @brief
 *
 * @param multiboot
 * @param magic
 *
 */
extern "C" void start(void* multiboot, u32 magic);

#endif // CORE_KERNEL_HPP_
