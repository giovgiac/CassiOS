/**
 * physical.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The frame allocator is Rust (kernel/src/memory/physical.rs).
 * PhysicalMemoryManager forwards to it until its callers are ported.
 *
 */

#ifndef MEMORY_PHYSICAL_HPP_
#define MEMORY_PHYSICAL_HPP_

#include <std/types.hpp>

#include <memory/multiboot.hpp>

extern "C" void physical_init(const cassio::memory::MultibootInfo* info);
extern "C" void physical_release_modules(const cassio::memory::MultibootInfo* info);

namespace cassio {
namespace memory {

static constexpr std::u32 FRAME_SIZE = 4096;

class PhysicalMemoryManager {
public:
    inline static PhysicalMemoryManager& getManager() { return instance; }

    void init(MultibootInfo* multibootInfo) { physical_init(multibootInfo); }

    /** Frees the module images; call once every module is loaded. */
    void releaseModules(MultibootInfo* multibootInfo) { physical_release_modules(multibootInfo); }

    PhysicalMemoryManager(const PhysicalMemoryManager&) = delete;
    PhysicalMemoryManager(PhysicalMemoryManager&&) = delete;
    PhysicalMemoryManager& operator=(const PhysicalMemoryManager&) = delete;
    PhysicalMemoryManager& operator=(PhysicalMemoryManager&&) = delete;

private:
    PhysicalMemoryManager() = default;

    static PhysicalMemoryManager instance;
};

} // namespace memory
} // namespace cassio

#endif // MEMORY_PHYSICAL_HPP_
