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

#include <std/alloc.hpp>

namespace cassio {
namespace memory {

static constexpr std::u32 KERNEL_HEAP_FRAMES = 256;
static constexpr std::u32 KERNEL_HEAP_SIZE = KERNEL_HEAP_FRAMES * 4096;

class KernelHeap final {
  public:
    inline static std::alloc::HeapAllocator& getAllocator() { return *instance; }

    static void init();

    KernelHeap() = delete;

  private:
    static std::alloc::HeapAllocator* instance;
};

} // namespace memory
} // namespace cassio

#endif // MEMORY_HEAP_HPP_
