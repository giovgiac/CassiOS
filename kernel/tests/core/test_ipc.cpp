#include <core/syscall.hpp>
#include <core/process.hpp>
#include <core/scheduler.hpp>
#include <test.hpp>

using namespace cassio;
using namespace cassio::kernel;

extern GlobalDescriptorTable test_gdt;

// -- Send queue tests --

TEST(ipc_send_queue_push_pop) {
    ProcessManager& pm = ProcessManager::getManager();
    Process* p = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(p != nullptr);

    ASSERT_EQ(p->sendQueueCount, 0u);
    ASSERT(p->sendQueuePush(1));
    ASSERT(p->sendQueuePush(2));
    ASSERT(p->sendQueuePush(3));
    ASSERT_EQ(p->sendQueueCount, 3u);

    ASSERT_EQ(p->sendQueuePop(), 1u);
    ASSERT_EQ(p->sendQueuePop(), 2u);
    ASSERT_EQ(p->sendQueuePop(), 3u);
    ASSERT_EQ(p->sendQueueCount, 0u);
    ASSERT_EQ(p->sendQueuePop(), 0u);  // Empty queue returns 0.

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
    ASSERT_EQ(p->sendQueueCount, 20u);

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
    for (u32 i = 0; i < 5; i++) p->sendQueuePush(i + 1);
    ASSERT_EQ(p->sendQueuePop(), 1u);
    ASSERT_EQ(p->sendQueuePop(), 2u);
    ASSERT_EQ(p->sendQueuePop(), 3u);

    for (u32 i = 0; i < 5; i++) p->sendQueuePush(i + 10);
    ASSERT_EQ(p->sendQueueCount, 7u);

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

    Message m1 = {1, 10, 20, 30, 40, 50};
    ASSERT(p->notifyPush(42, m1));

    u32 sender;
    Message out = {};
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
        Message m = {i, 0, 0, 0, 0, 0};
        ASSERT(p->notifyPush(i, m));
    }
    for (u32 i = 0; i < 40; i++) {
        u32 sender;
        Message out;
        ASSERT(p->notifyPop(sender, out));
        ASSERT_EQ(sender, i);
        ASSERT_EQ(out.type, i);
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
    Message recvBuf = {};
    receiver->state = ProcessState::ReceiveBlocked;
    receiver->msgPtr = (u32)&recvBuf;

    // Build a fake saved frame for the receiver so send() can set its return value.
    SyscallFrame recvFrame = {};
    receiver->esp = (u32)&recvFrame;

    // Set sender as current process and call send().
    pm.setCurrent(sender);
    Message sendMsg = { 42, 1, 2, 3, 4, 5 };
    i32 result = sh.send(receiver->pid, &sendMsg);

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

    receiver->state = ProcessState::Ready;  // Not ReceiveBlocked.

    pm.setCurrent(sender);
    Message sendMsg = { 99, 0, 0, 0, 0, 0 };
    i32 result = sh.send(receiver->pid, &sendMsg);

    ASSERT_EQ(result, 0);
    ASSERT(sender->state == ProcessState::SendBlocked);

    // Sender should be in receiver's send queue.
    ASSERT_EQ(receiver->sendQueueCount, 1u);
    ASSERT(receiver->sendHead != nullptr);
    ASSERT_EQ(receiver->sendHead->senderPid, sender->pid);

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
    sender->msg = { 77, 10, 20, 30, 40, 50 };
    receiver->sendQueuePush(sender->pid);

    pm.setCurrent(receiver);
    Message recvBuf = {};
    i32 result = sh.receive(&recvBuf);

    // Should deliver immediately (return sender PID).
    ASSERT_EQ(result, static_cast<i32>(sender->pid));
    ASSERT_EQ(recvBuf.type, 77u);
    ASSERT_EQ(recvBuf.arg1, 10u);
    ASSERT_EQ(recvBuf.arg5, 50u);

    // Receiver stays Ready (no blocking).
    ASSERT(receiver->state == ProcessState::Ready);
    ASSERT_EQ(receiver->sendQueueCount, 0u);

    pm.destroy(sender->pid);
    pm.destroy(receiver->pid);
}

TEST(ipc_receive_empty_queue_blocks) {
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* receiver = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(receiver != nullptr);

    pm.setCurrent(receiver);
    Message recvBuf = {};
    i32 result = sh.receive(&recvBuf);

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
    Message replyBuf = {};
    sender->msgPtr = (u32)&replyBuf;

    SyscallFrame senderFrame = {};
    sender->esp = (u32)&senderFrame;

    pm.setCurrent(replier);
    Message replyMsg = { 100, 11, 22, 33, 44, 55 };
    i32 result = sh.reply(sender->pid, &replyMsg);

    ASSERT_EQ(result, 0);
    ASSERT(sender->state == ProcessState::Ready);
    ASSERT_EQ(replyBuf.type, 100u);
    ASSERT_EQ(replyBuf.arg5, 55u);
    ASSERT_EQ(senderFrame.eax, 0u);  // Sender's return value = 0 (success).

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

    target->state = ProcessState::Ready;  // Not SendBlocked.

    pm.setCurrent(replier);
    Message replyMsg = { 0, 0, 0, 0, 0, 0 };
    i32 result = sh.reply(target->pid, &replyMsg);

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
    Message msg = { 1, 0, 0, 0, 0, 0 };
    i32 result = sh.send(p->pid, &msg);

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
    Message msg = { 1, 0, 0, 0, 0, 0 };
    i32 result = sh.send(99999, &msg);

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
    Message serverBuf = {};
    i32 result = sh.receive(&serverBuf);
    ASSERT_EQ(result, 0);
    ASSERT(server->state == ProcessState::ReceiveBlocked);

    // Build fake frame for server so send() can write its return value.
    SyscallFrame serverFrame = {};
    server->esp = (u32)&serverFrame;

    // Step 2: Client calls send() -- server is ReceiveBlocked, delivers immediately.
    pm.setCurrent(client);
    Message request = { 42, 1, 2, 3, 4, 5 };
    result = sh.send(server->pid, &request);
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
    Message response = { 99, 10, 20, 30, 40, 50 };
    result = sh.reply(client->pid, &response);
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
