/**
 * gdt.hpp
 * 
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
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
 * Contains null, kernel, user, and TSS segment descriptors. Loads the GDT
 * via lgdt, reloads segment registers, and loads the TSS on construction.
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

    /**
     * @brief The 104-byte Task State Segment read by the CPU on privilege transitions.
     *
     * Only esp0 and ss0 are used -- the CPU loads SS:ESP from these fields when
     * an interrupt or syscall transitions from ring 3 to ring 0.
     *
     */
    struct __attribute__((packed)) TaskStateSegment {
        u32 prev_tss;
        u32 esp0;
        u32 ss0;
        u32 esp1;
        u32 ss1;
        u32 esp2;
        u32 ss2;
        u32 cr3;
        u32 eip;
        u32 eflags;
        u32 eax;
        u32 ecx;
        u32 edx;
        u32 ebx;
        u32 esp;
        u32 ebp;
        u32 esi;
        u32 edi;
        u32 es;
        u32 cs;
        u32 ss;
        u32 ds;
        u32 fs;
        u32 gs;
        u32 ldt;
        u16 trap;
        u16 iomap_base;
    };

public:
    /**
     * @brief Builds the GDT with null, kernel, user, and TSS segments,
     *        then loads it via lgdt and the TSS via ltr.
     *
     */
    GlobalDescriptorTable();

    /**
     * @brief Destroys the GDT.
     *
     */
    ~GlobalDescriptorTable();

    /**
     * @brief Returns the byte offset of the kernel code segment descriptor.
     *
     */
    u16 getCodeOffset();

    /**
     * @brief Returns the byte offset of the kernel data segment descriptor.
     *
     */
    u16 getDataOffset();

    /**
     * @brief Returns the byte offset of the user code segment descriptor.
     *
     */
    u16 getUserCodeOffset();

    /**
     * @brief Returns the byte offset of the user data segment descriptor.
     *
     */
    u16 getUserDataOffset();

    /**
     * @brief Returns the byte offset of the TSS descriptor.
     *
     */
    u16 getTssOffset();

    /**
     * @brief Updates TSS.esp0, the kernel stack pointer used on ring 3 to ring 0 transitions.
     *
     */
    void setTssEsp0(u32 esp0);

    /** Deleted Methods */
    GlobalDescriptorTable(const GlobalDescriptorTable&) = delete;
    GlobalDescriptorTable(GlobalDescriptorTable&&) = delete;
    GlobalDescriptorTable& operator=(const GlobalDescriptorTable&) = delete;
    GlobalDescriptorTable& operator=(GlobalDescriptorTable&&) = delete;

private:
    SegmentDescriptor nullSegment;
    SegmentDescriptor codeSegment;
    SegmentDescriptor dataSegment;
    SegmentDescriptor userCodeSegment;
    SegmentDescriptor userDataSegment;
    SegmentDescriptor tssDescriptor;

    TaskStateSegment tss;
};

} // kernel
} // cassio

#endif // CORE_GDT_HPP_
