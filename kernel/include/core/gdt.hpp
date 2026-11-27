/**
 * gdt.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The GDT and TSS are Rust (kernel/src/core/gdt.rs). GlobalDescriptorTable
 * forwards to them until its callers are ported.
 *
 */

#ifndef CORE_GDT_HPP_
#define CORE_GDT_HPP_

#include <std/types.hpp>

extern "C" void gdt_init();
extern "C" void gdt_set_kernel_stack(std::u32 esp0);

namespace cassio {
namespace kernel {

class GlobalDescriptorTable {
public:
    /**
     * @brief Builds the GDT with null, kernel, user, and TSS segments,
     *        then loads it via lgdt and the TSS via ltr.
     *
     * There is one table: every instance shares it, and only the first
     * loads it.
     *
     */
    GlobalDescriptorTable() { gdt_init(); }

    /** Segment selectors (byte offsets of the descriptors). */
    std::u16 getCodeOffset() { return 0x08; }
    std::u16 getDataOffset() { return 0x10; }
    std::u16 getUserCodeOffset() { return 0x18; }
    std::u16 getUserDataOffset() { return 0x20; }
    std::u16 getTssOffset() { return 0x28; }

    /**
     * @brief Updates TSS.esp0, the kernel stack pointer used on ring 3 to ring 0 transitions.
     *
     */
    void setTssEsp0(std::u32 esp0) { gdt_set_kernel_stack(esp0); }

    /** Deleted Methods */
    GlobalDescriptorTable(const GlobalDescriptorTable&) = delete;
    GlobalDescriptorTable(GlobalDescriptorTable&&) = delete;
    GlobalDescriptorTable& operator=(const GlobalDescriptorTable&) = delete;
    GlobalDescriptorTable& operator=(GlobalDescriptorTable&&) = delete;
};

} // namespace kernel
} // namespace cassio

#endif // CORE_GDT_HPP_
