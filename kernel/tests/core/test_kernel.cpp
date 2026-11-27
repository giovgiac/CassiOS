#include <std/io.hpp>
#include <std/test.hpp>

#include <core/gdt.hpp>
#include <core/kernel.hpp>
#include <hardware/interrupt.hpp>
#include <hardware/serial.hpp>
#include <memory/heap.hpp>
#include <memory/paging.hpp>
#include <memory/physical.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;
using namespace cassio::hardware;
using namespace cassio::memory;
using namespace std::io;

void ctors() {
    for (ctor* ct = &start_ctors; ct != &end_ctors; ++ct) {
        (*ct)();
    }
}

GlobalDescriptorTable test_gdt;

// The Rust kernel tests (kernel/src/lib.rs), run before the C++ ones.
extern "C" void kernel_run_tests(u32* passed, u32* failed);

static void kernel_write(const char* buf, u32 len) {
    COM1::write(buf, len);
}

void start(void* multiboot, u32 magic) {
    COM1::init();

    GlobalDescriptorTable& gdt = test_gdt;
    InterruptManager& im = InterruptManager::getManager();
    im.load(gdt);

    PhysicalMemoryManager& pmm = PhysicalMemoryManager::getManager();
    pmm.init((MultibootInfo*)multiboot);

    KernelHeap::init();

    PagingManager& paging = PagingManager::getManager();
    paging.init((MultibootInfo*)multiboot);

    test::init(kernel_write);
    u32 passed = 0, failed = 0;
    kernel_run_tests(&passed, &failed);
    failed = test::run(passed, failed);

    // Exit QEMU: 0x00 -> exit code 1 (pass), 0x01 -> exit code 3 (fail)
    Port<u8> debug_exit(PortType::QemuDebugExit);
    debug_exit.write(failed > 0 ? 0x01 : 0x00);
}
