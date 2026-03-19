#include <std/msg.hpp>
#include <test.hpp>

using namespace std;

TEST(msg_struct_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(msg::Message)), (u32)24);
}

TEST(msg_type_constants_unique) {
    // Verify a selection of message type constants have expected values
    ASSERT_EQ(msg::MessageType::IrqNotify, (u32)1);
    ASSERT_EQ(msg::MessageType::NsRegister, (u32)2);
    ASSERT_EQ(msg::MessageType::KbdRead, (u32)4);
    ASSERT_EQ(msg::MessageType::VgaPutchar, (u32)5);
    ASSERT_EQ(msg::MessageType::VfsMkdir, (u32)10);
    ASSERT_EQ(msg::MessageType::MouseRead, (u32)17);
    ASSERT_EQ(msg::MessageType::AtaRead, (u32)18);
    ASSERT_EQ(msg::MessageType::NsListAll, (u32)20);
}

TEST(msg_fields_zero_initialized) {
    msg::Message m = {};
    ASSERT_EQ(m.type, (u32)0);
    ASSERT_EQ(m.arg1, (u32)0);
    ASSERT_EQ(m.arg2, (u32)0);
    ASSERT_EQ(m.arg3, (u32)0);
    ASSERT_EQ(m.arg4, (u32)0);
    ASSERT_EQ(m.arg5, (u32)0);
}
