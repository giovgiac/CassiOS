/**
 * new.hpp -- placement new operator
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_NEW_HPP_
#define COMMON_NEW_HPP_

#include <std/types.hpp>

inline void* operator new(std::usize, void* ptr) {
    return ptr;
}

#endif // COMMON_NEW_HPP_
