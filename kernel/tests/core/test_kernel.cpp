#include <core/gdt.hpp>
#include <core/kernel.hpp>
#include <hardware/interrupt.hpp>
#include <hardware/serial.hpp>
#include <std/io.hpp>
#include <memory/heap.hpp>
#include <memory/paging.hpp>
#include <memory/physical.hpp>
#include <test.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;
using namespace cassio::hardware;
using namespace cassio::memory;

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

GlobalDescriptorTable test_gdt;

static void kernel_write(const char* buf, u32 len) {
    Serial& com1 = COM1::getSerial();
    for (u32 i = 0; i < len; i++) {
        com1.putchar(buf[i]);
    }
}

void start(void* multiboot, u32 magic) {
    GlobalDescriptorTable& gdt = test_gdt;
    InterruptManager& im = InterruptManager::getManager();
    im.load(gdt);

    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    pmm.init((MultibootInfo*)multiboot);

    KernelHeap::init();

    PagingManager& paging = PagingManager::getManager();
    paging.init((MultibootInfo*)multiboot);

    test::init(kernel_write);
    u32 failed = test::run();

    // Exit QEMU: 0x00 -> exit code 1 (pass), 0x01 -> exit code 3 (fail)
    Port<u8> debug_exit(PortType::QemuDebugExit);
    debug_exit.write(failed > 0 ? 0x01 : 0x00);
}
