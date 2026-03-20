#include <std/ipc.hpp>
#include <std/test.hpp>

using namespace std;

TEST(ipc_message_struct_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(ipc::Message)), (u32)24);
}

TEST(ipc_message_type_constants) {
    ASSERT_EQ(ipc::MessageType::IrqNotify, (u32)1);
    ASSERT_EQ(ipc::MessageType::NsRegister, (u32)2);
    ASSERT_EQ(ipc::MessageType::KbdRead, (u32)4);
    ASSERT_EQ(ipc::MessageType::VfsMkdir, (u32)10);
    ASSERT_EQ(ipc::MessageType::MouseRead, (u32)17);
    ASSERT_EQ(ipc::MessageType::AtaRead, (u32)18);
    ASSERT_EQ(ipc::MessageType::NsListAll, (u32)20);
}

TEST(ipc_message_fields_zero_initialized) {
    ipc::Message m = {};
    ASSERT_EQ(m.type, (u32)0);
    ASSERT_EQ(m.arg1, (u32)0);
    ASSERT_EQ(m.arg2, (u32)0);
    ASSERT_EQ(m.arg3, (u32)0);
    ASSERT_EQ(m.arg4, (u32)0);
    ASSERT_EQ(m.arg5, (u32)0);
}
