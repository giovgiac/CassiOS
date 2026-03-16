#include <hardware/interrupt.hpp>
#include <hardware/irq.hpp>
#include <hardware/driver.hpp>
#include <core/gdt.hpp>
#include <test.hpp>

using namespace cassio;
using namespace cassio::hardware;
using namespace cassio::kernel;

TEST(interrupt_flags_values) {
    ASSERT_EQ(static_cast<u32>(IDT_DESCRIPTOR_PRESENT), 0x80u);
    ASSERT_EQ(static_cast<u32>(IDT_INTERRUPT_GATE), 0x0Eu);
    ASSERT_EQ(static_cast<u32>(IDT_TRAP_GATE), 0x0Fu);
}

TEST(interrupt_irq_offset) {
    ASSERT_EQ(static_cast<u32>(IRQ_OFFSET), 0x20u);
}

TEST(interrupt_driver_type_keyboard) {
    ASSERT_EQ(static_cast<u32>(DriverType::KeyboardController), 0x21u);
}

TEST(interrupt_driver_type_mouse) {
    ASSERT_EQ(static_cast<u32>(DriverType::MouseController), 0x2Cu);
}

TEST(interrupt_driver_type_timer) {
    ASSERT_EQ(static_cast<u32>(DriverType::SystemTimer), 0x20u);
}

extern cassio::kernel::GlobalDescriptorTable test_gdt;

TEST(interrupt_idt_size_and_base) {
    // IDT should have 256 entries of 8 bytes each.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    ASSERT_EQ(static_cast<u32>(idtr.limit), 256u * 8u - 1u);
    ASSERT(idtr.base != 0);
}

TEST(interrupt_idt_code_offset_matches_gdt) {
    GlobalDescriptorTable& gdt = test_gdt;

    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    // Check an IRQ entry (0x21 keyboard) -- code_offset should match GDT.
    u8* entry = reinterpret_cast<u8*>(idtr.base) + 0x21 * 8;
    u16 code_offset = *reinterpret_cast<u16*>(entry + 2);
    ASSERT_EQ(static_cast<u32>(code_offset), static_cast<u32>(gdt.getCodeOffset()));

    // Check an exception entry (0x00 div-by-zero) -- same code_offset.
    u8* exc_entry = reinterpret_cast<u8*>(idtr.base) + 0x00 * 8;
    u16 exc_code_offset = *reinterpret_cast<u16*>(exc_entry + 2);
    ASSERT_EQ(static_cast<u32>(exc_code_offset), static_cast<u32>(gdt.getCodeOffset()));
}

TEST(interrupt_unused_vectors_are_ignore) {
    // Unused vectors should all point to the same ignore handler.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    // Vectors 3 and 4 are unused.
    u8* entry3 = reinterpret_cast<u8*>(idtr.base) + 3 * 8;
    u8* entry4 = reinterpret_cast<u8*>(idtr.base) + 4 * 8;

    u32 addr3 = static_cast<u32>(*reinterpret_cast<u16*>(entry3 + 6)) << 16
              | static_cast<u32>(*reinterpret_cast<u16*>(entry3));
    u32 addr4 = static_cast<u32>(*reinterpret_cast<u16*>(entry4 + 6)) << 16
              | static_cast<u32>(*reinterpret_cast<u16*>(entry4));

    ASSERT_EQ(addr3, addr4);
    ASSERT(addr3 != 0);
}
