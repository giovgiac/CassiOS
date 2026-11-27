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
extern "C" std::u32 paging_create_address_space();
extern "C" void paging_destroy_address_space(std::u32 directory);

namespace cassio {
namespace memory {

class PagingManager {
public:
    inline static PagingManager& getManager() { return instance; }

    void init(MultibootInfo* multibootInfo) { paging_init(multibootInfo); }

    /**
     * @brief Creates an address space sharing the kernel mappings.
     *
     * Returns the physical address of its page directory, or 0 on failure.
     */
    std::u32 createAddressSpace() { return paging_create_address_space(); }

    /** Frees all user page tables, mapped user frames, and the directory. */
    void destroyAddressSpace(std::u32 pdPhysical) { paging_destroy_address_space(pdPhysical); }

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
