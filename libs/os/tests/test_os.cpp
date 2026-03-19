#include <std/os.hpp>
#include <std/test.hpp>

using namespace std;

TEST(os_syscall_constants_unique) {
    ASSERT(os::syscall::Send != os::syscall::Receive);
    ASSERT(os::syscall::Write != os::syscall::Sleep);
    ASSERT(os::syscall::Reboot != os::syscall::Shutdown);
}

TEST(os_syscall_constants_sequential) {
    ASSERT_EQ(os::syscall::Send, (u32)0);
    ASSERT_EQ(os::syscall::Receive, (u32)1);
    ASSERT_EQ(os::syscall::Reply, (u32)2);
    ASSERT_EQ(os::syscall::Exit, (u32)9);
    ASSERT_EQ(os::syscall::ProcList, (u32)14);
    ASSERT_EQ(os::syscall::Exec, (u32)15);
    ASSERT_EQ(os::syscall::WaitPid, (u32)16);
    ASSERT_EQ(os::syscall::Count, (u32)17);
}

TEST(os_proc_entry_size) {
    ASSERT_EQ(sizeof(os::ProcEntry), (usize)12);
}
