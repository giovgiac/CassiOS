#include <test.hpp>
#include <message.hpp>
#include <ipc.hpp>
#include <ns.hpp>

using namespace cassio;

TEST(kbd_ipc_read_empty_buffer) {
    u32 kbd_pid = Nameserver::lookup("kbd");
    ASSERT(kbd_pid != 0);

    Message msg = {};
    msg.type = MessageType::KbdRead;
    IPC::send(kbd_pid, &msg);

    // No keys pressed, buffer should be empty.
    ASSERT_EQ(msg.arg1, 0u);
}
