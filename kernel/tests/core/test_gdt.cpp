#include <core/gdt.hpp>
#include <test.hpp>

using namespace cassio;
using namespace std;
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
    // Verify against known offsets (code=0x08, data=0x10) rather than
    // constructing a new GDT on the stack, which would leave the GDTR
    // pointing at dead stack memory after the test returns.
    u16 cs, ds, ss;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%ss, %0" : "=r"(ss));

    ASSERT_EQ(static_cast<u32>(cs), 0x08u);
    ASSERT_EQ(static_cast<u32>(ds), 0x10u);
    ASSERT_EQ(static_cast<u32>(ss), 0x10u);
}

TEST(gdt_segment_descriptor_limit_large) {
    // 4 GiB - 1 = 0xFFFFFFFF
    // Granularity bit set: limit stored as pages, decoded with | 0xFFF
    GlobalDescriptorTable::SegmentDescriptor desc(0, 0xFFFFFFFF, 0x9A);
    u32 limit = desc.getLimit();
    // 0xFFFFFFFF -> (0xFFFFFFFF >> 12) = 0xFFFFF pages -> decoded: (0xFFFFF << 12) | 0xFFF = 0xFFFFFFFF
    ASSERT_EQ(limit, 0xFFFFFFFFu);
}

TEST(gdt_user_code_segment_offset) {
    // User code segment is the 4th entry -> offset 0x18
    u32 expected = 3 * sizeof(GlobalDescriptorTable::SegmentDescriptor);
    ASSERT_EQ(expected, 0x18u);
}

TEST(gdt_user_data_segment_offset) {
    // User data segment is the 5th entry -> offset 0x20
    u32 expected = 4 * sizeof(GlobalDescriptorTable::SegmentDescriptor);
    ASSERT_EQ(expected, 0x20u);
}

TEST(gdt_tss_offset) {
    // TSS descriptor is the 6th entry -> offset 0x28
    u32 expected = 5 * sizeof(GlobalDescriptorTable::SegmentDescriptor);
    ASSERT_EQ(expected, 0x28u);
}

TEST(gdt_tss_struct_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(GlobalDescriptorTable::TaskStateSegment)), 104u);
}

TEST(gdt_tss_loaded) {
    // Verify the task register holds the TSS selector (0x28).
    u16 tr;
    asm volatile("str %0" : "=r"(tr));
    ASSERT_EQ(static_cast<u32>(tr), 0x28u);
}

TEST(gdt_set_tss_esp0) {
    // Write a known value to TSS.esp0 via the GDT API, then verify by
    // reading through the hardware: SGDT -> TSS descriptor -> TSS.esp0.
    extern GlobalDescriptorTable test_gdt;
    test_gdt.setTssEsp0(0xDEADBEEF);

    // Read GDT base via SGDT.
    u8 gdtr[6];
    asm volatile("sgdt %0" : "=m"(gdtr));
    u32 gdtBase = *(u32*)(gdtr + 2);

    // TSS descriptor is at offset 0x28. Decode its base address.
    u8* tssDesc = (u8*)(gdtBase + 0x28);
    u32 tssBase = tssDesc[2] | (tssDesc[3] << 8) | (tssDesc[4] << 16) | (tssDesc[7] << 24);

    // esp0 is at offset 4 in the TSS struct.
    u32 esp0 = *(u32*)(tssBase + 4);
    ASSERT_EQ(esp0, 0xDEADBEEFu);
}
