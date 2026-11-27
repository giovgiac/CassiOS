/**
 * operators.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Global new and delete, backed by the Rust kernel heap.
 *
 */

#include "memory/heap.hpp"

void* operator new(std::usize size) {
    return kernel_alloc(size);
}

void* operator new[](std::usize size) {
    return kernel_alloc(size);
}

void operator delete(void* ptr) {
    kernel_free(ptr);
}

void operator delete[](void* ptr) {
    kernel_free(ptr);
}

void operator delete(void* ptr, std::usize) {
    kernel_free(ptr);
}

void operator delete[](void* ptr, std::usize) {
    kernel_free(ptr);
}
