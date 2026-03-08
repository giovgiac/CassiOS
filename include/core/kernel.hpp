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

#include <common/types.hpp>
#include <core/gdt.hpp>
#include <drivers/keyboard.hpp>
#include <drivers/mouse.hpp>
#include <hardware/interrupt.hpp>
#include <hardware/port.hpp>
#include <std/iostream.hpp>

/**
 * @brief Function pointer type for global constructors.
 *
 */
typedef void (*ctor)();

extern "C" ctor start_ctors;
extern "C" ctor end_ctors;

/**
 * @brief Invokes all global constructors between start_ctors and end_ctors.
 *
 */
extern "C" void ctors();

/**
 * @brief Kernel entry point called by the bootloader after global constructors.
 *
 * @param multiboot Pointer to the Multiboot information structure.
 * @param magic Multiboot magic number used to verify a valid boot.
 *
 */
extern "C" void start(void* multiboot, cassio::u32 magic);

#endif // CORE_KERNEL_HPP_
