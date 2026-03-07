#include <common/types.hpp>
#include <hardware/serial.hpp>
#include <hardware/port.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::hardware;

typedef void (*ctor)();
extern "C" ctor start_ctors;
extern "C" ctor end_ctors;

extern "C" void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

extern "C" void start(void* multiboot, u32 magic) {
    serial_init();

    u32 passed = 0, failed = 0;
    for (test::TestNode* t = test::test_list_head; t; t = t->next) {
        bool test_failed = false;
        t->fn(t->name, test_failed);
        if (!test_failed) {
            serial_puts("[PASS] ");
            serial_puts(t->name);
            serial_putchar('\n');
            passed++;
        } else {
            failed++;
        }
    }

    serial_puts("[DONE] ");
    serial_put_dec(passed);
    serial_puts(" passed, ");
    serial_put_dec(failed);
    serial_puts(" failed\n");

    // Exit QEMU: 0x00 -> exit code 1 (pass), 0x01 -> exit code 3 (fail)
    Port<u8> debug_exit(static_cast<u16>(0xf4));
    debug_exit.write(failed > 0 ? 0x01 : 0x00);
}
