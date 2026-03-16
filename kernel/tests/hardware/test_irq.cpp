#include <hardware/irq.hpp>
#include <hardware/driver.hpp>
#include <drivers/pit.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::hardware;
using namespace cassio::drivers;

TEST(irq_dispatch_to_pit) {
    // handleIrq(0x20) should dispatch to PitTimer and increment ticks.
    IrqManager& irq = IrqManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    u32 before = pit.getTicks();
    irq.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    ASSERT_EQ(after, before + 1);
}

TEST(irq_unregister_stops_dispatch) {
    IrqManager& irq = IrqManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    // Unregister PitTimer from IRQ 0.
    irq.unregisterDriver(static_cast<u8>(DriverType::SystemTimer), &pit);

    u32 before = pit.getTicks();
    irq.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    // Ticks should NOT have incremented.
    ASSERT_EQ(after, before);

    // Re-register so other tests are unaffected.
    irq.registerDriver(static_cast<u8>(DriverType::SystemTimer), &pit);
}

TEST(irq_register_restores_dispatch) {
    IrqManager& irq = IrqManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    // Unregister, re-register, then verify dispatch works.
    irq.unregisterDriver(static_cast<u8>(DriverType::SystemTimer), &pit);
    irq.registerDriver(static_cast<u8>(DriverType::SystemTimer), &pit);

    u32 before = pit.getTicks();
    irq.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    ASSERT_EQ(after, before + 1);
}

TEST(irq_unregister_wrong_driver_is_noop) {
    IrqManager& irq = IrqManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    // Unregister with a different pointer -- should be a no-op.
    Driver* fake = reinterpret_cast<Driver*>(0xDEADBEEF);
    irq.unregisterDriver(static_cast<u8>(DriverType::SystemTimer), fake);

    // PitTimer should still be registered and dispatched to.
    u32 before = pit.getTicks();
    irq.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    ASSERT_EQ(after, before + 1);
}

TEST(irq_handleirq_returns_esp_when_no_driver) {
    IrqManager& irq = IrqManager::getManager();

    // Vector 0x25 (IRQ 5) has no driver registered.
    u32 esp = 0x12345678;
    u32 result = irq.handleIrq(0x25, esp);

    // Should return esp unchanged.
    ASSERT_EQ(result, esp);
}

TEST(irq_idt_entries_are_distinct) {
    // Each registered IRQ vector should have a unique handler address.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    u8 vectors[] = { 0x20, 0x21, 0x2C, 0x2E };
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
