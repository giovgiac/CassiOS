#include <core/syscall.hpp>
#include <drivers/pit.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::drivers;

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
    // Test sleep via C++ method directly -- using int 0x80 would require
    // hardware interrupts enabled (sti), which conflicts with ATA tests.
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();
    PitTimer& pit = PitTimer::getTimer();

    u32 before = pit.getTicks();
    i32 result = sh.handleSyscall(SyscallNumber::Sleep, 0, 0, 0);
    ASSERT_EQ(result, 0);

    // With 0ms sleep, ticks should not change.
    u32 after = pit.getTicks();
    ASSERT_EQ(after, before);
}

TEST(syscall_write_returns_length) {
    const char* msg = "test";
    i32 result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SyscallNumber::Write), "b"(1u), "c"((u32)msg), "d"(4u));
    ASSERT_EQ(result, 4);
}

TEST(syscall_write_invalid_fd) {
    const char* msg = "x";
    i32 result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SyscallNumber::Write), "b"(99u), "c"((u32)msg), "d"(1u));
    ASSERT_EQ(result, static_cast<i32>(-1));
}

TEST(syscall_read_no_input) {
    char buf[16];
    i32 result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SyscallNumber::Read), "b"(0u), "c"((u32)buf), "d"(16u));
    ASSERT_EQ(result, 0);
}

TEST(syscall_read_invalid_fd) {
    char buf[16];
    i32 result;
    asm volatile("int $0x80" : "=a"(result)
                 : "a"(SyscallNumber::Read), "b"(99u), "c"((u32)buf), "d"(16u));
    ASSERT_EQ(result, static_cast<i32>(-1));
}

TEST(syscall_invalid_number) {
    i32 result;
    asm volatile("int $0x80" : "=a"(result) : "a"(999u));
    ASSERT_EQ(result, static_cast<i32>(-1));
}
