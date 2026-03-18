#include <hardware/exception.hpp>
#include <hardware/interrupt.hpp>
#include <test.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::hardware;

TEST(exception_vectors_have_distinct_handlers) {
    // Each exception vector should have a unique handler address.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    u8 vectors[] = { 0x00, 0x06, 0x0D, 0x0E };
    u32 addrs[4];

    for (u32 i = 0; i < 4; ++i) {
        u8* entry = reinterpret_cast<u8*>(idtr.base) + vectors[i] * 8;
        addrs[i] = static_cast<u32>(*reinterpret_cast<u16*>(entry + 6)) << 16
                 | static_cast<u32>(*reinterpret_cast<u16*>(entry));
        ASSERT(addrs[i] != 0);
    }

    // All four should be distinct from each other.
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = i + 1; j < 4; ++j) {
            ASSERT(addrs[i] != addrs[j]);
        }
    }
}

TEST(exception_vectors_differ_from_irq_vectors) {
    // Exception handlers should not be the same as any IRQ handler.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    u8 exc_vectors[] = { 0x00, 0x06, 0x0D, 0x0E };
    u8 irq_vectors[] = { 0x20, 0x21, 0x2C, 0x2E };

    for (u8 ev : exc_vectors) {
        u8* e_entry = reinterpret_cast<u8*>(idtr.base) + ev * 8;
        u32 e_addr = static_cast<u32>(*reinterpret_cast<u16*>(e_entry + 6)) << 16
                   | static_cast<u32>(*reinterpret_cast<u16*>(e_entry));

        for (u8 iv : irq_vectors) {
            u8* i_entry = reinterpret_cast<u8*>(idtr.base) + iv * 8;
            u32 i_addr = static_cast<u32>(*reinterpret_cast<u16*>(i_entry + 6)) << 16
                       | static_cast<u32>(*reinterpret_cast<u16*>(i_entry));

            ASSERT(e_addr != i_addr);
        }
    }
}

TEST(exception_unregistered_vectors_use_ignore) {
    // Vectors not registered as exceptions should use the ignore stub.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    // Vector 1 (debug) is not registered -- should be the ignore stub.
    u8* entry1 = reinterpret_cast<u8*>(idtr.base) + 1 * 8;
    u32 addr1 = static_cast<u32>(*reinterpret_cast<u16*>(entry1 + 6)) << 16
              | static_cast<u32>(*reinterpret_cast<u16*>(entry1));

    // Vector 2 (NMI) is also not registered.
    u8* entry2 = reinterpret_cast<u8*>(idtr.base) + 2 * 8;
    u32 addr2 = static_cast<u32>(*reinterpret_cast<u16*>(entry2 + 6)) << 16
              | static_cast<u32>(*reinterpret_cast<u16*>(entry2));

    // Both should use the same ignore handler.
    ASSERT_EQ(addr1, addr2);

    // And vector 0 (div-by-zero) should be different from them.
    u8* entry0 = reinterpret_cast<u8*>(idtr.base) + 0 * 8;
    u32 addr0 = static_cast<u32>(*reinterpret_cast<u16*>(entry0 + 6)) << 16
              | static_cast<u32>(*reinterpret_cast<u16*>(entry0));

    ASSERT(addr0 != addr1);
}

TEST(exception_syscall_gate_is_dpl3_trap) {
    // Vector 0x80 should be a DPL=3 trap gate (access byte 0xEF).
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    u8* entry = reinterpret_cast<u8*>(idtr.base) + 0x80 * 8;
    // 0xEF = IDT_DESCRIPTOR_PRESENT(0x80) | DPL=3(0x60) | IDT_TRAP_GATE(0x0F)
    ASSERT_EQ(static_cast<u32>(entry[5]), 0xEFu);

    // Handler should be non-zero.
    u32 handler_addr = static_cast<u32>(*reinterpret_cast<u16*>(entry + 6)) << 16
                     | static_cast<u32>(*reinterpret_cast<u16*>(entry));
    ASSERT(handler_addr != 0);
}
