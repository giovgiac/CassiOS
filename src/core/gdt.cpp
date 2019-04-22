/**
 * gdt.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/gdt.hpp"

using namespace cassio::kernel;

/** Global Descriptor Table Methods */

GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegment(0, 0, 0), unusedSegment(0, 0, 0), 
      codeSegment(0, 64 * 1024 * 1024, 0x9A), dataSegment(0, 64 * 1024 * 1024, 0x92) {
    usize i[2];
    i[1] = (usize)this;
    i[0] = sizeof(GlobalDescriptorTable) << 16;

    asm volatile("lgdt  (%0)": :"p" ((u8*)i + 2));
}

GlobalDescriptorTable::~GlobalDescriptorTable() {}

u16 GlobalDescriptorTable::getCodeOffset() {
    return (u8*)&codeSegment - (u8*)this;
}

u16 GlobalDescriptorTable::getDataOffset() {
    return (u8*)&dataSegment - (u8*)this;
}

/** Segment Descriptor Methods */

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(u32 base, u32 limit, u8 flags) {
    u8* target = (u8*)this;

    if (limit <= 65536) {
        target[6] = 0x40;
    }
    else {
        if ((limit & 0xFFF) != 0xFFF) {
            limit = (limit >> 12) - 1;
        }
        else {
            limit = limit >> 12;
        }

        target[6] = 0xC0;
    }

    // Encode Limit
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] |= (limit >> 16) & 0xF;

    // Encode Base
    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF;
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;

    // Encode Access
    target[5] = flags;
}

u32 GlobalDescriptorTable::SegmentDescriptor::getBase() {
    u8* target = (u8*)this;

    // Decode Base
    u32 result = target[7];
    result = (result << 8) + target[4];
    result = (result << 8) + target[3];
    result = (result << 8) + target[2];

    return result;
}

u32 GlobalDescriptorTable::SegmentDescriptor::getLimit() {
    u8* target = (u8*)this;

    // Decode Limit
    u32 result = target[6] & 0xF;
    result = (result << 8) + target[1];
    result = (result << 8) + target[0];

    if ((target[6] & 0xC0) == 0xC0) {
        result = (result << 12) | 0xFFF;
    }

    return result;
}
