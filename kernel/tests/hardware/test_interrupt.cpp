#include <hardware/interrupt.hpp>
#include <hardware/irq.hpp>
#include <hardware/driver.hpp>
#include <core/gdt.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::hardware;
using namespace cassio::kernel;

TEST(interrupt_flags_values) {
    ASSERT_EQ(static_cast<u32>(IDT_DESCRIPTOR_PRESENT), 0x80u);
    ASSERT_EQ(static_cast<u32>(IDT_INTERRUPT_GATE), 0x0Eu);
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

TEST(interrupt_idt_entry_after_load) {
    GlobalDescriptorTable& gdt = test_gdt;

    // IDT is already loaded by test kernel's start() -- do not call
    // im.load() here as it resets all 256 entries, wiping the syscall gate.

    // Retrieve IDT base via sidt
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    // IDT should have 256 entries of 8 bytes each
    ASSERT_EQ(static_cast<u32>(idtr.limit), 256u * 8u - 1u);

    // Check entry 0x21 (keyboard IRQ): code_offset should match GDT code segment
    u8* entry = reinterpret_cast<u8*>(idtr.base) + 0x21 * 8;
    u16 code_offset = *reinterpret_cast<u16*>(entry + 2);
    ASSERT_EQ(static_cast<u32>(code_offset), static_cast<u32>(gdt.getCodeOffset()));

    // Access byte should be IDT_DESCRIPTOR_PRESENT | IDT_INTERRUPT_GATE = 0x8E
    ASSERT_EQ(static_cast<u32>(entry[5]), 0x8Eu);

    // Reserved byte should be 0
    ASSERT_EQ(static_cast<u32>(entry[4]), 0u);

    // Handler address should be non-zero
    u32 handler_addr = static_cast<u32>(*reinterpret_cast<u16*>(entry + 6)) << 16
                     | static_cast<u32>(*reinterpret_cast<u16*>(entry));
    ASSERT(handler_addr != 0);
}

TEST(interrupt_exception_idt_entries) {
    // Verify exception vectors 0, 6, 13, 14 are set to non-ignore handlers.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    // Get the ignore handler address for comparison
    u8* ignore_entry = reinterpret_cast<u8*>(idtr.base) + 1 * 8; // vector 1 is unused
    u32 ignore_addr = static_cast<u32>(*reinterpret_cast<u16*>(ignore_entry + 6)) << 16
                    | static_cast<u32>(*reinterpret_cast<u16*>(ignore_entry));

    u8 vectors[] = { 0x00, 0x06, 0x0D, 0x0E };
    for (u8 v : vectors) {
        u8* entry = reinterpret_cast<u8*>(idtr.base) + v * 8;
        u32 handler_addr = static_cast<u32>(*reinterpret_cast<u16*>(entry + 6)) << 16
                         | static_cast<u32>(*reinterpret_cast<u16*>(entry));

        // Handler should be set (non-zero) and different from the ignore stub
        ASSERT(handler_addr != 0);
        ASSERT(handler_addr != ignore_addr);

        // Access byte should be 0x8E (present, DPL=0, interrupt gate)
        ASSERT_EQ(static_cast<u32>(entry[5]), 0x8Eu);
    }
}
