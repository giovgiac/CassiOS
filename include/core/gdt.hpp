/**
 * gdt.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef CORE_GDT_HPP_
#define CORE_GDT_HPP_

#include <common/types.hpp>

namespace cassio {
namespace kernel {

/**
 * @brief Manages the x86 Global Descriptor Table for memory segmentation.
 *
 * Contains null, code, and data segment descriptors. Loads the GDT via lgdt
 * and reloads all segment registers on construction.
 *
 */
class GlobalDescriptorTable {
public:
    /**
     * @brief An 8-byte packed entry describing a memory segment in the GDT.
     *
     * Encodes base address, limit, and access flags in the format required
     * by the x86 processor. Handles both 16-bit and page-granularity limits.
     *
     */
    class __attribute__((packed)) SegmentDescriptor {
    private:
        u16 limit_low;
        u16 base_low;
        u8  base_high;
        u8  flags;
        u8  limit_high;
        u8  base_very_high;

    public:
        /**
         * @brief Constructs a segment descriptor with the given base, limit, and access flags.
         *
         */
        SegmentDescriptor(u32 base, u32 limit, u8 flags);

        /**
         * @brief Decodes and returns the 32-bit base address from this descriptor.
         *
         */
        u32 getBase();

        /**
         * @brief Decodes and returns the segment limit from this descriptor.
         *
         */
        u32 getLimit();

    };

public:
    /**
     * @brief Builds the GDT with null, code, and data segments, then loads it via lgdt.
     *
     */
    GlobalDescriptorTable();

    /**
     * @brief Destroys the GDT.
     *
     */
    ~GlobalDescriptorTable();

    /**
     * @brief Returns the byte offset of the code segment descriptor within the GDT.
     *
     */
    u16 getCodeOffset();

    /**
     * @brief Returns the byte offset of the data segment descriptor within the GDT.
     *
     */
    u16 getDataOffset();

    /** Deleted Methods */
    GlobalDescriptorTable(const GlobalDescriptorTable&) = delete;
    GlobalDescriptorTable(GlobalDescriptorTable&&) = delete;
    GlobalDescriptorTable& operator=(const GlobalDescriptorTable&) = delete;
    GlobalDescriptorTable& operator=(GlobalDescriptorTable&&) = delete;

private:
    SegmentDescriptor nullSegment;
    SegmentDescriptor codeSegment;
    SegmentDescriptor dataSegment;
};

} // kernel
} // cassio

#endif // CORE_GDT_HPP_
