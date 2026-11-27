/**
 * elf.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The loader is Rust (kernel/src/core/elf.rs). ElfLoader forwards to it
 * until its callers are ported.
 *
 */

#ifndef CORE_ELF_HPP_
#define CORE_ELF_HPP_

#include <std/types.hpp>

extern "C" bool elf_load(std::u32 directory, const std::u8* data, std::u32 size, std::u32* entry,
                         std::u32* heapStart);

namespace cassio {
namespace kernel {

struct ElfLoadResult {
    std::u32 entryPoint;
    std::u32 heapStart; // Page-aligned end of loaded segments (initial sbrk break).
    bool success;
};

class ElfLoader {
public:
    /**
     * @brief Loads an executable into the address space at pdPhysical.
     *
     * On failure some pages may already be mapped: destroy the address space.
     */
    static ElfLoadResult load(std::u32 pdPhysical, const std::u8* elfData, std::u32 elfSize) {
        ElfLoadResult result = {0, 0, false};
        result.success =
            elf_load(pdPhysical, elfData, elfSize, &result.entryPoint, &result.heapStart);
        return result;
    }
};

} // namespace kernel
} // namespace cassio

#endif // CORE_ELF_HPP_
