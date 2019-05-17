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
 * @brief
 *
 */
typedef void (*ctor)();

extern "C" ctor start_ctors;
extern "C" ctor end_ctors;

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
extern "C" void start(void* multiboot, cassio::u32 magic);

#endif // CORE_KERNEL_HPP_
