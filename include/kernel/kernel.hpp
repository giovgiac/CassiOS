/**
 * kernel.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 *
 */

#ifndef KERNEL_HPP_
#define KERNEL_HPP_

typedef void (*ctor)();

extern "C" ctor start_ctors;
extern "C" ctor end_ctors;

#endif // KERNEL_HPP_
