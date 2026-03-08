#include <core/gdt.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::kernel;

TEST(gdt_segment_descriptor_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(GlobalDescriptorTable::SegmentDescriptor)), 8u);
}

TEST(gdt_code_segment_offset) {
    // Code segment is the 2nd entry (null, code) -> offset 0x08
    u32 expected = 1 * sizeof(GlobalDescriptorTable::SegmentDescriptor);
    ASSERT_EQ(expected, 0x08u);
}

TEST(gdt_data_segment_offset) {
    // Data segment is the 3rd entry (null, code, data) -> offset 0x10
    u32 expected = 2 * sizeof(GlobalDescriptorTable::SegmentDescriptor);
    ASSERT_EQ(expected, 0x10u);
}

TEST(gdt_segment_descriptor_base) {
    GlobalDescriptorTable::SegmentDescriptor desc(0x12345678, 65536, 0x9A);
    ASSERT_EQ(desc.getBase(), 0x12345678u);
}

TEST(gdt_segment_descriptor_limit_small) {
    GlobalDescriptorTable::SegmentDescriptor desc(0, 65536, 0x9A);
    ASSERT_EQ(desc.getLimit(), 65536u);
}

TEST(gdt_segment_registers_after_load) {
    GlobalDescriptorTable gdt;

    u16 cs, ds, ss;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%ss, %0" : "=r"(ss));

    ASSERT_EQ(static_cast<u32>(cs), static_cast<u32>(gdt.getCodeOffset()));
    ASSERT_EQ(static_cast<u32>(ds), static_cast<u32>(gdt.getDataOffset()));
    ASSERT_EQ(static_cast<u32>(ss), static_cast<u32>(gdt.getDataOffset()));
}

TEST(gdt_segment_descriptor_limit_large) {
    // 128 MiB = 128 * 1024 * 1024 = 0x08000000
    // Granularity bit set: limit stored as pages, decoded with | 0xFFF
    GlobalDescriptorTable::SegmentDescriptor desc(0, 128 * 1024 * 1024, 0x9A);
    u32 limit = desc.getLimit();
    // The limit should round-trip through the page granularity encoding
    // 128 MiB -> (128*1024*1024 >> 12) - 1 = 0x7FFF pages -> decoded: (0x7FFF << 12) | 0xFFF = 0x07FFFFFF
    ASSERT_EQ(limit, 0x07FFFFFFu);
}
