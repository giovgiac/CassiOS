/**
 * operators.cpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "memory/heap.hpp"

using namespace cassio;
using namespace cassio::memory;

void* operator new(usize size) {
    return HeapAllocator::getAllocator().allocate(size);
}

void* operator new[](usize size) {
    return HeapAllocator::getAllocator().allocate(size);
}

void operator delete(void* ptr) {
    HeapAllocator::getAllocator().free(ptr);
}

void operator delete[](void* ptr) {
    HeapAllocator::getAllocator().free(ptr);
}

void operator delete(void* ptr, usize) {
    HeapAllocator::getAllocator().free(ptr);
}

void operator delete[](void* ptr, usize) {
    HeapAllocator::getAllocator().free(ptr);
}
