/**
 * operators.cpp -- userspace global operator new/delete
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <userheap.hpp>

using namespace cassio;

void* operator new(usize size) {
    return UserHeap::alloc(size);
}

void* operator new[](usize size) {
    return UserHeap::alloc(size);
}

void operator delete(void* ptr) {
    UserHeap::free(ptr);
}

void operator delete[](void* ptr) {
    UserHeap::free(ptr);
}

void operator delete(void* ptr, usize) {
    UserHeap::free(ptr);
}

void operator delete[](void* ptr, usize) {
    UserHeap::free(ptr);
}
