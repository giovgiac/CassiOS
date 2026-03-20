/**
 * heap.cpp -- kernel heap initialization
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "memory/heap.hpp"

#include "memory/physical.hpp"
#include "memory/virtual.hpp"

using namespace cassio;
using namespace std;
using namespace cassio::memory;

alloc::HeapAllocator* KernelHeap::instance = nullptr;
alignas(alloc::HeapAllocator) static u8 kernel_heap_storage[sizeof(alloc::HeapAllocator)];

void KernelHeap::init() {
    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();

    // allocFrame() returns a physical address; add KERNEL_VBASE to get
    // the virtual address in the direct map.
    void* base = (void*)((u32)pmm.allocFrame() + KERNEL_VBASE);
    if (base == (void*)KERNEL_VBASE) {
        return;
    }

    for (u32 i = 1; i < KERNEL_HEAP_FRAMES; i++) {
        pmm.allocFrame();
    }

    instance = new (kernel_heap_storage) alloc::HeapAllocator(base, KERNEL_HEAP_SIZE);
}
