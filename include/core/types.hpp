/**
 * types.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_TYPES_HPP_
#define CORE_TYPES_HPP_

// Float Types
typedef     float               f32;
typedef     double              f64;

// Integer Types
typedef     char                i8;
typedef     unsigned char       u8;
typedef     short               i16;
typedef     unsigned short      u16;
typedef     int                 i32;
typedef     unsigned int        u32;
typedef     long long           i64;
typedef     unsigned long long  u64;

// Size Types
typedef     int                 isize;
typedef     unsigned int        usize;

// Guarantee Expected Sizes
static_assert(sizeof(f32) == 4, "Unexpected size for f32");
static_assert(sizeof(f64) == 8, "Unexpected size for f64");
static_assert(sizeof(i8) == 1, "Unexpected size for i8");
static_assert(sizeof(u8) == 1, "Unexpected size for u8");
static_assert(sizeof(i16) == 2, "Unexpected size for i16");
static_assert(sizeof(u16) == 2, "Unexpected size for u16");
static_assert(sizeof(i32) == 4, "Unexpected size for i32");
static_assert(sizeof(u32) == 4, "Unexpected size for u32");
static_assert(sizeof(i64) == 8, "Unexpected size for i64");
static_assert(sizeof(u64) == 8, "Unexpected size for u64");
static_assert(sizeof(isize) == 4, "Unexpected size for isize");
static_assert(sizeof(usize) == 4, "Unexpected size for usize");

#endif // CORE_TYPES_HPP_
