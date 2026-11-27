#include <std/os.hpp>
#include <std/test.hpp>

using namespace std;

TEST(os_syscall_constants_unique) {
    ASSERT(os::syscall::Send != os::syscall::Receive);
    ASSERT(os::syscall::Write != os::syscall::Sleep);
    ASSERT(os::syscall::Reboot != os::syscall::Shutdown);
}

TEST(os_syscall_constants_sequential) {
    // Every value, mirrored in libs/sys/src/os.rs.
    ASSERT_EQ(os::syscall::Send, (u32)0);
    ASSERT_EQ(os::syscall::Receive, (u32)1);
    ASSERT_EQ(os::syscall::Reply, (u32)2);
    ASSERT_EQ(os::syscall::IrqRegister, (u32)3);
    ASSERT_EQ(os::syscall::Write, (u32)4);
    ASSERT_EQ(os::syscall::Sleep, (u32)5);
    ASSERT_EQ(os::syscall::Uptime, (u32)6);
    ASSERT_EQ(os::syscall::Reboot, (u32)7);
    ASSERT_EQ(os::syscall::Shutdown, (u32)8);
    ASSERT_EQ(os::syscall::Exit, (u32)9);
    ASSERT_EQ(os::syscall::MapDevice, (u32)10);
    ASSERT_EQ(os::syscall::Notify, (u32)11);
    ASSERT_EQ(os::syscall::MemInfo, (u32)12);
    ASSERT_EQ(os::syscall::Sbrk, (u32)13);
    ASSERT_EQ(os::syscall::ProcList, (u32)14);
    ASSERT_EQ(os::syscall::Exec, (u32)15);
    ASSERT_EQ(os::syscall::WaitPid, (u32)16);
    ASSERT_EQ(os::syscall::FramebufferInfo, (u32)17);
    ASSERT_EQ(os::syscall::Count, (u32)18);
}

TEST(os_proc_entry_size) {
    ASSERT_EQ(sizeof(os::ProcEntry), (usize)12);
}

TEST(os_framebuffer_info_size) {
    ASSERT_EQ(sizeof(os::FramebufferInfo), (usize)20);
}
