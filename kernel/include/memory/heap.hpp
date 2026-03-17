/**
 * heap.hpp -- kernel heap
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef MEMORY_HEAP_HPP_
#define MEMORY_HEAP_HPP_

#include <heap.hpp>

namespace cassio {
namespace memory {

// Re-export into cassio::memory for backward compatibility.
using cassio::BlockHeader;
using cassio::HeapAllocator;

static constexpr u32 KERNEL_HEAP_FRAMES = 256;
static constexpr u32 KERNEL_HEAP_SIZE = KERNEL_HEAP_FRAMES * 4096;

class KernelHeap final {
public:
    inline static HeapAllocator& getAllocator() {
        return *instance;
    }

    static void init();

    KernelHeap() = delete;

private:
    static HeapAllocator* instance;
};

} // memory
} // cassio

#endif // MEMORY_HEAP_HPP_
