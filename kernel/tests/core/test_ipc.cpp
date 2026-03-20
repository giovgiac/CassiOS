#include <std/test.hpp>

#include <core/process.hpp>
#include <core/scheduler.hpp>
#include <core/syscall.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;

extern GlobalDescriptorTable test_gdt;

// -- Send queue tests --

TEST(ipc_send_queue_push_pop) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    ASSERT_EQ(p->sendQueue.getCount(), 0u);
    ASSERT(p->sendQueuePush(1));
    ASSERT(p->sendQueuePush(2));
    ASSERT(p->sendQueuePush(3));
    ASSERT_EQ(p->sendQueue.getCount(), 3u);

    ASSERT_EQ(p->sendQueuePop(), 1u);
    ASSERT_EQ(p->sendQueuePop(), 2u);
    ASSERT_EQ(p->sendQueuePop(), 3u);
    ASSERT_EQ(p->sendQueue.getCount(), 0u);
    ASSERT_EQ(p->sendQueuePop(), 0u); // Empty queue returns 0.

    pm.destroy(p->pid);
}

TEST(ipc_send_queue_beyond_old_limit) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    // Should be able to push more than the old fixed limit of 15.
    for (u32 i = 0; i < 20; i++) {
        ASSERT(p->sendQueuePush(i + 1));
    }
    ASSERT_EQ(p->sendQueue.getCount(), 20u);

    // Drain and verify FIFO order.
    for (u32 i = 0; i < 20; i++) {
        ASSERT_EQ(p->sendQueuePop(), i + 1);
    }
    pm.destroy(p->pid);
}

TEST(ipc_send_queue_interleaved_push_pop) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    // Push 5, pop 3, push 5 more -- tests FIFO ordering.
    for (u32 i = 0; i < 5; i++)
        p->sendQueuePush(i + 1);
    ASSERT_EQ(p->sendQueuePop(), 1u);
    ASSERT_EQ(p->sendQueuePop(), 2u);
    ASSERT_EQ(p->sendQueuePop(), 3u);

    for (u32 i = 0; i < 5; i++)
        p->sendQueuePush(i + 10);
    ASSERT_EQ(p->sendQueue.getCount(), 7u);

    // FIFO order: 4, 5, 10, 11, 12, 13, 14
    ASSERT_EQ(p->sendQueuePop(), 4u);
    ASSERT_EQ(p->sendQueuePop(), 5u);
    ASSERT_EQ(p->sendQueuePop(), 10u);

    pm.destroy(p->pid);
}

// -- Notification queue tests --

TEST(ipc_notify_queue_push_pop) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    ipc::Message m1 = {1, 10, 20, 30, 40, 50};
    ASSERT(p->notifyPush(42, m1));

    u32 sender;
    ipc::Message out = {};
    ASSERT(p->notifyPop(sender, out));
    ASSERT_EQ(sender, 42u);
    ASSERT_EQ(out.type, 1u);
    ASSERT_EQ(out.arg1, 10u);
    ASSERT_EQ(out.arg5, 50u);

    // Queue is empty now.
    ASSERT(!p->notifyPop(sender, out));

    pm.destroy(p->pid);
}

TEST(ipc_notify_queue_beyond_old_limit) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    // Should be able to push more than the old fixed limit of 32.
    for (u32 i = 0; i < 40; i++) {
        ipc::Message m = {i, 0, 0, 0, 0, 0};
        ASSERT(p->notifyPush(i, m));
    }
    for (u32 i = 0; i < 40; i++) {
        u32 sender;
        ipc::Message out;
        ASSERT(p->notifyPop(sender, out));
        ASSERT_EQ(sender, i);
        ASSERT_EQ(out.type, i);
    }

    pm.destroy(p->pid);
}

TEST(ipc_notify_queue_with_data) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    u8 sendData[32];
    for (u32 i = 0; i < 32; i++)
        sendData[i] = static_cast<u8>(i + 1);

    ipc::Message m = {5, 32, 0, 0, 0, 0};
    ASSERT(p->notifyPush(7, m, sendData, 32));

    u32 sender;
    ipc::Message out = {};
    u8 recvData[64] = {};
    ASSERT(p->notifyPop(sender, out, recvData, 64));
    ASSERT_EQ(sender, 7u);
    ASSERT_EQ(out.type, 5u);
    ASSERT_EQ(out.arg1, 32u);
    for (u32 i = 0; i < 32; i++) {
        ASSERT_EQ(recvData[i], static_cast<u8>(i + 1));
    }

    pm.destroy(p->pid);
}

TEST(ipc_notify_queue_data_truncated) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    u8 sendData[32];
    for (u32 i = 0; i < 32; i++)
        sendData[i] = static_cast<u8>(i + 100);

    ipc::Message m = {1, 0, 0, 0, 0, 0};
    ASSERT(p->notifyPush(1, m, sendData, 32));

    u32 sender;
    ipc::Message out = {};
    u8 recvData[16] = {};
    ASSERT(p->notifyPop(sender, out, recvData, 16));
    // Only first 16 bytes should be copied.
    for (u32 i = 0; i < 16; i++) {
        ASSERT_EQ(recvData[i], static_cast<u8>(i + 100));
    }

    pm.destroy(p->pid);
}

// -- IPC send/receive/reply tests --

TEST(ipc_send_to_receive_blocked_delivers_immediately) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(receiver != nullptr);

    // Simulate receiver blocked in receive().
    ipc::Message recvBuf = {};
    receiver->state = ProcessState::ReceiveBlocked;
    receiver->msgPtr = (u32)&recvBuf;

    // Build a fake saved frame for the receiver so send() can set its return value.
    SyscallFrame recvFrame = {};
    receiver->esp = (u32)&recvFrame;

    // Set sender as current process and call send().
    pm.setCurrent(sender);
    ipc::Message sendMsg = {42, 1, 2, 3, 4, 5};
    i32 result = sh.send(receiver->pid, &sendMsg, 0, 0);

    // Sender should block.
    ASSERT_EQ(result, 0);
    ASSERT(sender->state == ProcessState::SendBlocked);

    // Receiver should be woken with the message.
    ASSERT(receiver->state == ProcessState::Ready);
    ASSERT_EQ(recvBuf.type, 42u);
    ASSERT_EQ(recvBuf.arg1, 1u);

    // Receiver's return value (EAX) should be sender's PID.
    ASSERT_EQ(recvFrame.eax, sender->pid);

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

TEST(ipc_send_to_non_blocked_queues_sender) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(receiver != nullptr);

    receiver->state = ProcessState::Ready; // Not ReceiveBlocked.

    pm.setCurrent(sender);
    ipc::Message sendMsg = {99, 0, 0, 0, 0, 0};
    i32 result = sh.send(receiver->pid, &sendMsg, 0, 0);

    ASSERT_EQ(result, 0);
    ASSERT(sender->state == ProcessState::SendBlocked);

    // Sender should be in receiver's send queue.
    ASSERT_EQ(receiver->sendQueue.getCount(), 1u);
    ASSERT(receiver->sendQueue.getHead() != nullptr);
    ASSERT_EQ(receiver->sendQueue.getHead()->senderPid, sender->pid);

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

TEST(ipc_receive_with_pending_sender) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(receiver != nullptr);

    // Simulate sender already in queue with a message.
    sender->state = ProcessState::SendBlocked;
    sender->msg = {77, 10, 20, 30, 40, 50};
    receiver->sendQueuePush(sender->pid);

    pm.setCurrent(receiver);
    ipc::Message recvBuf = {};
    i32 result = sh.receive(&recvBuf, 0, 0);

    // Should deliver immediately (return sender PID).
    ASSERT_EQ(result, static_cast<i32>(sender->pid));
    ASSERT_EQ(recvBuf.type, 77u);
    ASSERT_EQ(recvBuf.arg1, 10u);
    ASSERT_EQ(recvBuf.arg5, 50u);

    // Receiver stays Ready (no blocking).
    ASSERT(receiver->state == ProcessState::Ready);
    ASSERT_EQ(receiver->sendQueue.getCount(), 0u);

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

TEST(ipc_receive_empty_queue_blocks) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(receiver != nullptr);

    pm.setCurrent(receiver);
    ipc::Message recvBuf = {};
    i32 result = sh.receive(&recvBuf, 0, 0);

    ASSERT_EQ(result, 0);
    ASSERT(receiver->state == ProcessState::ReceiveBlocked);
    ASSERT_EQ(receiver->msgPtr, (u32)&recvBuf);

    pm.destroy(receiver->pid);
}

TEST(ipc_reply_unblocks_sender) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* replier = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(replier != nullptr);

    // Simulate sender blocked waiting for reply.
    sender->state = ProcessState::SendBlocked;
    ipc::Message replyBuf = {};
    sender->msgPtr = (u32)&replyBuf;

    SyscallFrame senderFrame = {};
    sender->esp = (u32)&senderFrame;

    pm.setCurrent(replier);
    ipc::Message replyMsg = {100, 11, 22, 33, 44, 55};
    i32 result = sh.reply(sender->pid, &replyMsg, 0, 0);

    ASSERT_EQ(result, 0);
    ASSERT(sender->state == ProcessState::Ready);
    ASSERT_EQ(replyBuf.type, 100u);
    ASSERT_EQ(replyBuf.arg5, 55u);
    ASSERT_EQ(senderFrame.eax, 0u); // Sender's return value = 0 (success).

    pm.destroy(sender->pid);
    pm.destroy(replier->pid);
}

TEST(ipc_reply_to_non_blocked_fails) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* target = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* replier = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(target != nullptr);
    ASSERT(replier != nullptr);

    target->state = ProcessState::Ready; // Not SendBlocked.

    pm.setCurrent(replier);
    ipc::Message replyMsg = {0, 0, 0, 0, 0, 0};
    i32 result = sh.reply(target->pid, &replyMsg, 0, 0);

    ASSERT_EQ(result, static_cast<i32>(-1));

    pm.destroy(target->pid);
    pm.destroy(replier->pid);
}

TEST(ipc_send_to_self_fails) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    pm.setCurrent(p);
    ipc::Message msg = {1, 0, 0, 0, 0, 0};
    i32 result = sh.send(p->pid, &msg, 0, 0);

    ASSERT_EQ(result, static_cast<i32>(-1));
    // Process should NOT be blocked.
    ASSERT(p->state != ProcessState::SendBlocked);

    pm.destroy(p->pid);
}

TEST(ipc_send_to_invalid_pid_fails) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    pm.setCurrent(p);
    ipc::Message msg = {1, 0, 0, 0, 0, 0};
    i32 result = sh.send(99999, &msg, 0, 0);

    ASSERT_EQ(result, static_cast<i32>(-1));

    pm.destroy(p->pid);
}

// -- Full round-trip test --

TEST(ipc_full_round_trip) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* client = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* server = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(client != nullptr);
    ASSERT(server != nullptr);

    // Step 1: Server calls receive() -- no pending senders, blocks.
    pm.setCurrent(server);
    ipc::Message serverBuf = {};
    i32 result = sh.receive(&serverBuf, 0, 0);
    ASSERT_EQ(result, 0);
    ASSERT(server->state == ProcessState::ReceiveBlocked);

    // Build fake frame for server so send() can write its return value.
    SyscallFrame serverFrame = {};
    server->esp = (u32)&serverFrame;

    // Step 2: Client calls send() -- server is ReceiveBlocked, delivers immediately.
    pm.setCurrent(client);
    ipc::Message request = {42, 1, 2, 3, 4, 5};
    result = sh.send(server->pid, &request, 0, 0);
    ASSERT_EQ(result, 0);
    ASSERT(client->state == ProcessState::SendBlocked);
    ASSERT(server->state == ProcessState::Ready);

    // Server received the message.
    ASSERT_EQ(serverBuf.type, 42u);
    ASSERT_EQ(serverBuf.arg1, 1u);
    // Server's return value = client PID.
    ASSERT_EQ(serverFrame.eax, client->pid);

    // Build fake frame for client so reply() can write its return value.
    SyscallFrame clientFrame = {};
    client->esp = (u32)&clientFrame;

    // Step 3: Server calls reply() -- unblocks client with response.
    pm.setCurrent(server);
    ipc::Message response = {99, 10, 20, 30, 40, 50};
    result = sh.reply(client->pid, &response, 0, 0);
    ASSERT_EQ(result, 0);
    ASSERT(client->state == ProcessState::Ready);

    // Client received the reply in its original send buffer.
    ASSERT_EQ(request.type, 99u);
    ASSERT_EQ(request.arg1, 10u);
    ASSERT_EQ(request.arg5, 50u);
    // Client's return value = 0 (success).
    ASSERT_EQ(clientFrame.eax, 0u);

    pm.destroy(client->pid);
    pm.destroy(server->pid);
}

// -- Long message tests (bulk data transfer) --

TEST(ipc_send_receive_blocked_with_data) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(receiver != nullptr);

    // Receiver is blocked with a data buffer.
    ipc::Message recvBuf = {};
    u8 recvData[64] = {};
    receiver->state = ProcessState::ReceiveBlocked;
    receiver->msgPtr = (u32)&recvBuf;
    receiver->dataPtr = (u32)recvData;
    receiver->dataLen = sizeof(recvData);

    SyscallFrame recvFrame = {};
    receiver->esp = (u32)&recvFrame;

    // Sender sends with data.
    u8 sendData[32];
    for (u32 i = 0; i < 32; i++)
        sendData[i] = static_cast<u8>(i + 1);

    pm.setCurrent(sender);
    ipc::Message sendMsg = {10, 32, 0, 0, 0, 0};
    i32 result = sh.send(receiver->pid, &sendMsg, (u32)sendData, sizeof(sendData));

    ASSERT_EQ(result, 0);
    ASSERT(receiver->state == ProcessState::Ready);
    ASSERT_EQ(recvBuf.type, 10u);
    ASSERT_EQ(recvBuf.arg1, 32u);

    // Verify data was copied.
    for (u32 i = 0; i < 32; i++) {
        ASSERT_EQ(recvData[i], static_cast<u8>(i + 1));
    }

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

TEST(ipc_send_data_truncated) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(receiver != nullptr);

    // Receiver has small buffer.
    ipc::Message recvBuf = {};
    u8 recvData[16] = {};
    receiver->state = ProcessState::ReceiveBlocked;
    receiver->msgPtr = (u32)&recvBuf;
    receiver->dataPtr = (u32)recvData;
    receiver->dataLen = sizeof(recvData);

    SyscallFrame recvFrame = {};
    receiver->esp = (u32)&recvFrame;

    // Sender sends more data than receiver can hold.
    u8 sendData[64];
    for (u32 i = 0; i < 64; i++)
        sendData[i] = static_cast<u8>(i + 10);

    pm.setCurrent(sender);
    ipc::Message sendMsg = {20, 64, 0, 0, 0, 0};
    i32 result = sh.send(receiver->pid, &sendMsg, (u32)sendData, sizeof(sendData));

    ASSERT_EQ(result, 0);

    // Only 16 bytes should be copied (receiver's capacity).
    for (u32 i = 0; i < 16; i++) {
        ASSERT_EQ(recvData[i], static_cast<u8>(i + 10));
    }

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

TEST(ipc_send_no_data_backward_compat) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(receiver != nullptr);

    // Receiver has a data buffer but sender sends no data.
    ipc::Message recvBuf = {};
    u8 recvData[32];
    for (u32 i = 0; i < 32; i++)
        recvData[i] = 0xFF;
    receiver->state = ProcessState::ReceiveBlocked;
    receiver->msgPtr = (u32)&recvBuf;
    receiver->dataPtr = (u32)recvData;
    receiver->dataLen = sizeof(recvData);

    SyscallFrame recvFrame = {};
    receiver->esp = (u32)&recvFrame;

    pm.setCurrent(sender);
    ipc::Message sendMsg = {42, 0, 0, 0, 0, 0};
    i32 result = sh.send(receiver->pid, &sendMsg, 0, 0);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(recvBuf.type, 42u);

    // Receiver's data buffer should be untouched.
    for (u32 i = 0; i < 32; i++) {
        ASSERT_EQ(recvData[i], static_cast<u8>(0xFF));
    }

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

TEST(ipc_reply_with_data) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* replier = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(replier != nullptr);

    // Simulate sender blocked with a data buffer for reply.
    u8 senderData[64] = {};
    sender->state = ProcessState::SendBlocked;
    ipc::Message replyBuf = {};
    sender->msgPtr = (u32)&replyBuf;
    sender->dataPtr = (u32)senderData;
    sender->dataLen = sizeof(senderData);

    SyscallFrame senderFrame = {};
    sender->esp = (u32)&senderFrame;

    // Replier sends reply with data.
    u8 replyData[48];
    for (u32 i = 0; i < 48; i++)
        replyData[i] = static_cast<u8>(i + 50);

    pm.setCurrent(replier);
    ipc::Message replyMsg = {200, 48, 0, 0, 0, 0};
    i32 result = sh.reply(sender->pid, &replyMsg, (u32)replyData, sizeof(replyData));

    ASSERT_EQ(result, 0);
    ASSERT(sender->state == ProcessState::Ready);
    ASSERT_EQ(replyBuf.type, 200u);
    ASSERT_EQ(senderFrame.eax, 0u);

    // Verify reply data in sender's buffer.
    for (u32 i = 0; i < 48; i++) {
        ASSERT_EQ(senderData[i], static_cast<u8>(i + 50));
    }

    pm.destroy(sender->pid);
    pm.destroy(replier->pid);
}

TEST(ipc_full_round_trip_with_data) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* client = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* server = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(client != nullptr);
    ASSERT(server != nullptr);

    // Step 1: Server blocks in receive with data buffer.
    u8 serverData[128] = {};
    pm.setCurrent(server);
    ipc::Message serverBuf = {};
    i32 result = sh.receive(&serverBuf, (u32)serverData, sizeof(serverData));
    ASSERT_EQ(result, 0);
    ASSERT(server->state == ProcessState::ReceiveBlocked);

    SyscallFrame serverFrame = {};
    server->esp = (u32)&serverFrame;

    // Step 2: Client sends with data.
    u8 clientData[64];
    for (u32 i = 0; i < 64; i++)
        clientData[i] = static_cast<u8>(i);

    pm.setCurrent(client);
    ipc::Message request = {42, 64, 0, 0, 0, 0};
    result = sh.send(server->pid, &request, (u32)clientData, sizeof(clientData));
    ASSERT_EQ(result, 0);
    ASSERT(server->state == ProcessState::Ready);

    // Server got the message and data.
    ASSERT_EQ(serverBuf.type, 42u);
    ASSERT_EQ(serverBuf.arg1, 64u);
    for (u32 i = 0; i < 64; i++) {
        ASSERT_EQ(serverData[i], static_cast<u8>(i));
    }

    SyscallFrame clientFrame = {};
    client->esp = (u32)&clientFrame;

    // Step 3: Server replies with data.
    u8 replyData[32];
    for (u32 i = 0; i < 32; i++)
        replyData[i] = static_cast<u8>(i + 100);

    pm.setCurrent(server);
    ipc::Message response = {99, 32, 0, 0, 0, 0};
    result = sh.reply(client->pid, &response, (u32)replyData, sizeof(replyData));
    ASSERT_EQ(result, 0);
    ASSERT(client->state == ProcessState::Ready);

    // Client received reply message and data.
    ASSERT_EQ(request.type, 99u);
    ASSERT_EQ(request.arg1, 32u);
    for (u32 i = 0; i < 32; i++) {
        ASSERT_EQ(clientData[i], static_cast<u8>(i + 100));
    }

    pm.destroy(client->pid);
    pm.destroy(server->pid);
}

TEST(ipc_receive_queued_sender_with_data) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* sender = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(sender != nullptr);
    ASSERT(receiver != nullptr);

    // Simulate sender already in queue with data.
    u8 senderData[24];
    for (u32 i = 0; i < 24; i++)
        senderData[i] = static_cast<u8>(i + 200);

    sender->state = ProcessState::SendBlocked;
    sender->msg = {77, 24, 0, 0, 0, 0};
    sender->dataPtr = (u32)senderData;
    sender->dataLen = sizeof(senderData);
    receiver->sendQueuePush(sender->pid);

    // Receiver calls receive with data buffer.
    u8 recvData[64] = {};
    pm.setCurrent(receiver);
    ipc::Message recvBuf = {};
    i32 result = sh.receive(&recvBuf, (u32)recvData, sizeof(recvData));

    ASSERT_EQ(result, static_cast<i32>(sender->pid));
    ASSERT_EQ(recvBuf.type, 77u);
    for (u32 i = 0; i < 24; i++) {
        ASSERT_EQ(recvData[i], static_cast<u8>(i + 200));
    }

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

// -- Scheduler integration tests --

TEST(scheduler_skips_send_blocked) {
    Scheduler& s = Scheduler::getScheduler();
    s.reset();
    s.init(test_gdt);

    ProcessManager& pm = ProcessManager::getManager();
    Process* p1 = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* p2 = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    p1->state = ProcessState::Running;
    p2->state = ProcessState::SendBlocked;
    pm.setCurrent(p1);

    // Even after time slice, scheduler should skip p2 and stay on p1.
    for (u32 i = 0; i < Scheduler::DEFAULT_TIME_SLICE; i++) {
        s.schedule(0x5000);
    }
    ASSERT(p1->state == ProcessState::Running);

    pm.destroy(p1->pid);
    pm.destroy(p2->pid);
}

TEST(scheduler_skips_receive_blocked) {
    Scheduler& s = Scheduler::getScheduler();
    s.reset();
    s.init(test_gdt);

    ProcessManager& pm = ProcessManager::getManager();
    Process* p1 = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* p2 = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    p1->state = ProcessState::Running;
    p2->state = ProcessState::ReceiveBlocked;
    pm.setCurrent(p1);

    for (u32 i = 0; i < Scheduler::DEFAULT_TIME_SLICE; i++) {
        s.schedule(0x5000);
    }
    ASSERT(p1->state == ProcessState::Running);

    pm.destroy(p1->pid);
    pm.destroy(p2->pid);
}

TEST(scheduler_reschedule_switches_to_ready) {
    Scheduler& s = Scheduler::getScheduler();
    s.reset();
    s.init(test_gdt);

    ProcessManager& pm = ProcessManager::getManager();
    Process* p1 = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* p2 = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    p1->state = ProcessState::Running;
    p2->state = ProcessState::Ready;
    pm.setCurrent(p1);

    // Mark p1 as SendBlocked and reschedule.
    p1->state = ProcessState::SendBlocked;
    u32 newEsp = s.reschedule(0x5000);

    // Should switch to p2.
    ASSERT_EQ(newEsp, 0x4000u);
    ASSERT(p2->state == ProcessState::Running);
    ASSERT(p1->state == ProcessState::SendBlocked);

    pm.destroy(p1->pid);
    pm.destroy(p2->pid);
}
