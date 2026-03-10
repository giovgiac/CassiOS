/**
 * operators.cpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "memory/heap.hpp"

using namespace cassio::memory;

void* operator new(cassio::usize size) {
    return KernelHeap::getAllocator().allocate(size);
}

void* operator new[](cassio::usize size) {
    return KernelHeap::getAllocator().allocate(size);
}

void operator delete(void* ptr) {
    KernelHeap::getAllocator().free(ptr);
}

void operator delete[](void* ptr) {
    KernelHeap::getAllocator().free(ptr);
}

void operator delete(void* ptr, cassio::usize) {
    KernelHeap::getAllocator().free(ptr);
}

void operator delete[](void* ptr, cassio::usize) {
    KernelHeap::getAllocator().free(ptr);
}
