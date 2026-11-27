#include <std/ipc.hpp>
#include <std/test.hpp>

using namespace std;

TEST(ipc_message_struct_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(ipc::Message)), (u32)24);
}

TEST(ipc_message_type_constants) {
    // Every value, mirrored in libs/sys/src/ipc.rs.
    ASSERT_EQ(ipc::MessageType::IrqNotify, (u32)1);
    ASSERT_EQ(ipc::MessageType::NsRegister, (u32)2);
    ASSERT_EQ(ipc::MessageType::NsLookup, (u32)3);
    ASSERT_EQ(ipc::MessageType::NsListAll, (u32)4);
    ASSERT_EQ(ipc::MessageType::KbdRead, (u32)5);
    ASSERT_EQ(ipc::MessageType::VfsMkdir, (u32)6);
    ASSERT_EQ(ipc::MessageType::VfsRemove, (u32)7);
    ASSERT_EQ(ipc::MessageType::VfsOpen, (u32)8);
    ASSERT_EQ(ipc::MessageType::VfsRead, (u32)9);
    ASSERT_EQ(ipc::MessageType::VfsWrite, (u32)10);
    ASSERT_EQ(ipc::MessageType::VfsList, (u32)11);
    ASSERT_EQ(ipc::MessageType::VfsStat, (u32)12);
    ASSERT_EQ(ipc::MessageType::VfsClose, (u32)13);
    ASSERT_EQ(ipc::MessageType::MouseRead, (u32)14);
    ASSERT_EQ(ipc::MessageType::AtaRead, (u32)15);
    ASSERT_EQ(ipc::MessageType::AtaWrite, (u32)16);
    ASSERT_EQ(ipc::MessageType::DisplayFillRect, (u32)17);
    ASSERT_EQ(ipc::MessageType::DisplayDrawRect, (u32)18);
    ASSERT_EQ(ipc::MessageType::DisplayBlit, (u32)19);
    ASSERT_EQ(ipc::MessageType::DisplayScroll, (u32)20);
    ASSERT_EQ(ipc::MessageType::DisplayFlush, (u32)21);
    ASSERT_EQ(ipc::MessageType::DisplayGetInfo, (u32)22);
    ASSERT_EQ(ipc::MessageType::DisplayDrawChar, (u32)23);
    ASSERT_EQ(ipc::MessageType::TerminalPutchar, (u32)24);
    ASSERT_EQ(ipc::MessageType::TerminalWrite, (u32)25);
    ASSERT_EQ(ipc::MessageType::TerminalClear, (u32)26);
    ASSERT_EQ(ipc::MessageType::TerminalSetCursor, (u32)27);
    ASSERT_EQ(ipc::MessageType::TerminalGetCursor, (u32)28);
    ASSERT_EQ(ipc::MessageType::TerminalFlush, (u32)29);
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
