#include <std/test.hpp>

#include <core/gdt.hpp>

using namespace std;
using namespace cassio::kernel;

extern GlobalDescriptorTable test_gdt;

// The selectors the C++ side hands out must be the ones the Rust GDT
// loaded (kernel/src/core/gdt.rs). The user selectors are covered by every
// process reaching user mode.
TEST(gdt_selectors_match_the_loaded_registers) {
    u16 cs, ds, ss, tr;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    asm volatile("mov %%ds, %0" : "=r"(ds));
    asm volatile("mov %%ss, %0" : "=r"(ss));
    asm volatile("str %0" : "=r"(tr));
    ASSERT_EQ(static_cast<u32>(cs), static_cast<u32>(test_gdt.getCodeOffset()));
    ASSERT_EQ(static_cast<u32>(ds), static_cast<u32>(test_gdt.getDataOffset()));
    ASSERT_EQ(static_cast<u32>(ss), static_cast<u32>(test_gdt.getDataOffset()));
    ASSERT_EQ(static_cast<u32>(tr), static_cast<u32>(test_gdt.getTssOffset()));
    ASSERT_EQ(static_cast<u32>(test_gdt.getUserCodeOffset()), 0x18u);
    ASSERT_EQ(static_cast<u32>(test_gdt.getUserDataOffset()), 0x20u);
}
