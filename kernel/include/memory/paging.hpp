/**
 * paging.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef MEMORY_PAGING_HPP_
#define MEMORY_PAGING_HPP_

#include <types.hpp>
#include <memory/multiboot.hpp>

namespace cassio {
namespace memory {

static constexpr u16 PAGE_PRESENT    = 0x01;
static constexpr u16 PAGE_READWRITE  = 0x02;
static constexpr u16 PAGE_USER       = 0x04;

class PagingManager {
public:
    inline static PagingManager& getManager() {
        return instance;
    }

    void init(MultibootInfo* multibootInfo);

    void mapPage(u32 virtualAddr, u32 physicalAddr, u16 flags);
    void unmapPage(u32 virtualAddr);
    void flushTLB(u32 virtualAddr);

    /**
     * @brief Creates a new address space with shared kernel mappings.
     *
     * Allocates a page directory frame, copies kernel PDEs (768-1023)
     * from the kernel page directory, and zeroes user PDEs (0-767).
     * Returns the physical address of the new page directory, or 0 on failure.
     *
     */
    u32 createAddressSpace();

    /**
     * @brief Maps a page in an arbitrary page directory.
     *
     * Accesses the target page directory and page tables via phys + KERNEL_VBASE.
     *
     */
    void mapUserPage(u32 pdPhysical, u32 virtualAddr, u32 physicalAddr, u16 flags);

    /**
     * @brief Frees all user page tables, mapped user frames, and the page directory.
     *
     */
    void destroyAddressSpace(u32 pdPhysical);

    PagingManager(const PagingManager&) = delete;
    PagingManager(PagingManager&&) = delete;
    PagingManager& operator=(const PagingManager&) = delete;
    PagingManager& operator=(PagingManager&&) = delete;

private:
    PagingManager();

    static PagingManager instance;

    u32 pageDirectory[1024] __attribute__((aligned(4096)));
};

} // memory
} // cassio

#endif // MEMORY_PAGING_HPP_
