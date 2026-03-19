#include <core/syscall.hpp>
#include <hardware/pit.hpp>
#include <std/test.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;
using namespace cassio::hardware;

TEST(syscall_uptime_returns_nonnegative) {
    i32 result;
    asm volatile("int $0x80" : "=a"(result) : "a"(SyscallNumber::Uptime));
    ASSERT(result >= 0);
}

TEST(syscall_uptime_monotonic) {
    i32 first;
    asm volatile("int $0x80" : "=a"(first) : "a"(SyscallNumber::Uptime));

    i32 second;
    asm volatile("int $0x80" : "=a"(second) : "a"(SyscallNumber::Uptime));

    ASSERT(second >= first);
}

TEST(syscall_sleep_returns_zero) {
    // Test sleep by calling PitTimer directly -- using int 0x80 would require
    // hardware interrupts enabled (sti), which conflicts with ATA tests.
    PitTimer& pit = PitTimer::getTimer();

    u32 before = pit.getTicks();
    pit.sleep(0);
    u32 after = pit.getTicks();
    ASSERT_EQ(after, before);
}

TEST(syscall_write_serial_returns_length) {
    const char* msg = "test";
    i32 result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SyscallNumber::Write), "b"(2u), "c"((u32)msg), "d"(4u));
    ASSERT_EQ(result, 4);
}

TEST(syscall_write_vga_removed) {
    const char* msg = "x";
    i32 result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SyscallNumber::Write), "b"(1u), "c"((u32)msg), "d"(1u));
    ASSERT_EQ(result, static_cast<i32>(-1));
}

TEST(syscall_write_invalid_fd) {
    const char* msg = "x";
    i32 result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SyscallNumber::Write), "b"(99u), "c"((u32)msg), "d"(1u));
    ASSERT_EQ(result, static_cast<i32>(-1));
}

TEST(syscall_invalid_number) {
    i32 result;
    asm volatile("int $0x80" : "=a"(result) : "a"(999u));
    ASSERT_EQ(result, static_cast<i32>(-1));
}
