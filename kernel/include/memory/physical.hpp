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
extern "C" std::u32 physical_alloc_frame();
extern "C" void physical_free_frame(std::u32 address);
extern "C" bool physical_is_frame_used(std::u32 address);
extern "C" std::u32 physical_total_frames();
extern "C" std::u32 physical_free_frames();
extern "C" std::u32 physical_used_frames();

namespace cassio {
namespace memory {

static constexpr std::u32 FRAME_SIZE = 4096;

class PhysicalMemoryManager {
public:
    inline static PhysicalMemoryManager& getManager() { return instance; }

    void init(MultibootInfo* multibootInfo) { physical_init(multibootInfo); }

    /** Frees the module images; call once every module is loaded. */
    void releaseModules(MultibootInfo* multibootInfo) { physical_release_modules(multibootInfo); }

    /** A free frame's physical address, or null when memory is exhausted. */
    void* allocFrame() { return reinterpret_cast<void*>(physical_alloc_frame()); }
    void freeFrame(void* address) { physical_free_frame(reinterpret_cast<std::u32>(address)); }
    bool isFrameUsed(void* address) const {
        return physical_is_frame_used(reinterpret_cast<std::u32>(address));
    }

    std::u32 getTotalFrames() const { return physical_total_frames(); }
    std::u32 getUsedFrames() const { return physical_used_frames(); }
    std::u32 getFreeFrames() const { return physical_free_frames(); }

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
