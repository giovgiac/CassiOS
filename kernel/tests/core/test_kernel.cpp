#include <core/gdt.hpp>
#include <core/kernel.hpp>
#include <hardware/interrupt.hpp>
#include <hardware/serial.hpp>
#include <memory/heap.hpp>
#include <memory/paging.hpp>
#include <memory/physical.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::hardware;
using namespace cassio::memory;

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

GlobalDescriptorTable test_gdt;

void start(void* multiboot, u32 magic) {
    GlobalDescriptorTable& gdt = test_gdt;
    InterruptManager& im = InterruptManager::getManager();
    im.load(gdt);

    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    pmm.init((MultibootInfo*)multiboot);

    KernelHeap::init();

    PagingManager& paging = PagingManager::getManager();
    paging.init((MultibootInfo*)multiboot);

    Serial& com1 = COM1::getSerial();

    u32 passed = 0, failed = 0;
    for (test::TestNode* t = test::test_list_head; t; t = t->next) {
        bool test_failed = false;
        t->fn(t->name, test_failed);
        if (!test_failed) {
            com1.puts("[PASS] ");
            com1.puts(t->name);
            com1.putchar('\n');
            passed++;
        } else {
            failed++;
        }
    }

    com1.puts("[DONE] ");
    com1.put_dec(passed);
    com1.puts(" passed, ");
    com1.put_dec(failed);
    com1.puts(" failed\n");

    // Exit QEMU: 0x00 -> exit code 1 (pass), 0x01 -> exit code 3 (fail)
    Port<u8> debug_exit(PortType::QemuDebugExit);
    debug_exit.write(failed > 0 ? 0x01 : 0x00);
}
