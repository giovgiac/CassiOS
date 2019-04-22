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

#include <core/types.hpp>

namespace cassio {
namespace kernel {

/**
 * @brief 
 * 
 */
class GlobalDescriptorTable {
public:
    /**
     * @brief 
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
         * @brief
         * 
         */
        SegmentDescriptor(u32 base, u32 limit, u8 flags);

        /**
         * @brief
         * 
         */
        u32 getBase();

        /**
         * @brief
         * 
         */
        u32 getLimit();

    };

public:
    /**
     * @brief
     * 
     */
    GlobalDescriptorTable();

    /**
     * @brief
     * 
     */
    ~GlobalDescriptorTable();

    /**
     * @brief
     * 
     */
    u16 getCodeOffset();

    /**
     * @brief
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
    SegmentDescriptor unusedSegment;
    SegmentDescriptor codeSegment;
    SegmentDescriptor dataSegment;
};

} // kernel
} // cassio

#endif // CORE_GDT_HPP_
