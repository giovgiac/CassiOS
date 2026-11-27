/**
 * paging.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Paging is Rust (kernel/src/memory/paging.rs). PagingManager forwards to
 * it until its callers are ported.
 *
 */

#ifndef MEMORY_PAGING_HPP_
#define MEMORY_PAGING_HPP_

#include <std/types.hpp>

#include <memory/multiboot.hpp>

extern "C" void paging_init(const cassio::memory::MultibootInfo* info);
extern "C" void paging_map_page(std::u32 virt, std::u32 phys, std::u32 flags);
extern "C" void paging_unmap_page(std::u32 virt);
extern "C" void paging_flush_tlb(std::u32 virt);
extern "C" std::u32 paging_create_address_space();
extern "C" void paging_map_user_page(std::u32 directory, std::u32 virt, std::u32 phys,
                                     std::u32 flags);
extern "C" void paging_destroy_address_space(std::u32 directory);
extern "C" bool paging_user_range_mapped(std::u32 start, std::u32 len);

namespace cassio {
namespace memory {

static constexpr std::u16 PAGE_PRESENT = 0x01;
static constexpr std::u16 PAGE_READWRITE = 0x02;
static constexpr std::u16 PAGE_USER = 0x04;

class PagingManager {
public:
    inline static PagingManager& getManager() { return instance; }

    void init(MultibootInfo* multibootInfo) { paging_init(multibootInfo); }

    void mapPage(std::u32 virtualAddr, std::u32 physicalAddr, std::u16 flags) {
        paging_map_page(virtualAddr, physicalAddr, flags);
    }
    void unmapPage(std::u32 virtualAddr) { paging_unmap_page(virtualAddr); }
    void flushTLB(std::u32 virtualAddr) { paging_flush_tlb(virtualAddr); }

    /**
     * @brief Creates an address space sharing the kernel mappings.
     *
     * Returns the physical address of its page directory, or 0 on failure.
     */
    std::u32 createAddressSpace() { return paging_create_address_space(); }

    void mapUserPage(std::u32 pdPhysical, std::u32 virtualAddr, std::u32 physicalAddr,
                     std::u16 flags) {
        paging_map_user_page(pdPhysical, virtualAddr, physicalAddr, flags);
    }

    /** Frees all user page tables, mapped user frames, and the directory. */
    void destroyAddressSpace(std::u32 pdPhysical) { paging_destroy_address_space(pdPhysical); }

    /** Whether every page of the range is mapped user-accessible in CR3. */
    bool userRangeMapped(std::u32 start, std::u32 len) {
        return paging_user_range_mapped(start, len);
    }

    PagingManager(const PagingManager&) = delete;
    PagingManager(PagingManager&&) = delete;
    PagingManager& operator=(const PagingManager&) = delete;
    PagingManager& operator=(PagingManager&&) = delete;

private:
    PagingManager() = default;

    static PagingManager instance;
};

} // namespace memory
} // namespace cassio

#endif // MEMORY_PAGING_HPP_
