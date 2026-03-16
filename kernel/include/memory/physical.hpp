/**
 * physical.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef MEMORY_PHYSICAL_HPP_
#define MEMORY_PHYSICAL_HPP_

#include <types.hpp>
#include <memory/multiboot.hpp>

namespace cassio {
namespace memory {

static constexpr u32 FRAME_SIZE = 4096;
static constexpr u32 BITMAP_SIZE = 131072;

class PhysicalMemoryManager {
public:
    inline static PhysicalMemoryManager& getManager() {
        return instance;
    }

    void init(MultibootInfo* multibootInfo);

    void* allocFrame();
    void freeFrame(void* address);
    bool isFrameUsed(void* address) const;

    u32 getTotalFrames() const;
    u32 getUsedFrames() const;
    u32 getFreeFrames() const;

    PhysicalMemoryManager(const PhysicalMemoryManager&) = delete;
    PhysicalMemoryManager(PhysicalMemoryManager&&) = delete;
    PhysicalMemoryManager& operator=(const PhysicalMemoryManager&) = delete;
    PhysicalMemoryManager& operator=(PhysicalMemoryManager&&) = delete;

private:
    PhysicalMemoryManager();

    void markRegionUsed(u32 base, u32 length);
    void markRegionFree(u32 base, u32 length);

    static PhysicalMemoryManager instance;

    u8 bitmap[BITMAP_SIZE];
    u32 totalFrames;
};

} // memory
} // cassio

#endif // MEMORY_PHYSICAL_HPP_
