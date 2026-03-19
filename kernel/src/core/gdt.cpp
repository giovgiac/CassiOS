/**
 * gdt.cpp
 * 
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/gdt.hpp"
#include <std/mem.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;

/** Global Descriptor Table Methods */

GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegment(0, 0, 0),
      codeSegment(0, 0xFFFFFFFF, 0x9A),
      dataSegment(0, 0xFFFFFFFF, 0x92),
      userCodeSegment(0, 0xFFFFFFFF, 0xFA),
      userDataSegment(0, 0xFFFFFFFF, 0xF2),
      tssDescriptor((u32)&tss, sizeof(TaskStateSegment) - 1, 0x89) {
    // Fix TSS descriptor: SegmentDescriptor sets D/B=1 (0x40) for small limits,
    // but TSS descriptors must have D/B=0.
    u8* tssBytes = (u8*)&tssDescriptor;
    tssBytes[6] &= 0x0F;

    // Initialize TSS: zero all fields, then set the kernel stack segment.
    mem::set(&tss, 0, sizeof(TaskStateSegment));
    tss.ss0 = 0x10;
    tss.iomap_base = sizeof(TaskStateSegment);

    // Load GDT (limit covers only the 6 descriptors, not the TSS struct).
    usize gdtr[2];
    gdtr[1] = (usize)this;
    gdtr[0] = (6 * sizeof(SegmentDescriptor)) << 16;

    asm volatile(
        "lgdt  (%0)\n"
        "ljmp  $0x08, $1f\n"
        "1:\n"
        "mov   $0x10, %%ax\n"
        "mov   %%ax, %%ds\n"
        "mov   %%ax, %%es\n"
        "mov   %%ax, %%fs\n"
        "mov   %%ax, %%gs\n"
        "mov   %%ax, %%ss\n"
        : : "p" ((u8*)gdtr + 2)
        : "eax"
    );

    // Load TSS.
    asm volatile(
        "mov   $0x28, %%ax\n"
        "ltr   %%ax\n"
        : : : "eax"
    );
}

GlobalDescriptorTable::~GlobalDescriptorTable() {}

u16 GlobalDescriptorTable::getCodeOffset() {
    return (u8*)&codeSegment - (u8*)this;
}

u16 GlobalDescriptorTable::getDataOffset() {
    return (u8*)&dataSegment - (u8*)this;
}

u16 GlobalDescriptorTable::getUserCodeOffset() {
    return (u8*)&userCodeSegment - (u8*)this;
}

u16 GlobalDescriptorTable::getUserDataOffset() {
    return (u8*)&userDataSegment - (u8*)this;
}

u16 GlobalDescriptorTable::getTssOffset() {
    return (u8*)&tssDescriptor - (u8*)this;
}

void GlobalDescriptorTable::setTssEsp0(u32 esp0) {
    tss.esp0 = esp0;
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
