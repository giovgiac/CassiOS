#include <std/test.hpp>

#include <core/process.hpp>
#include <core/syscall.hpp>
#include <hardware/irq.hpp>
#include <hardware/pit.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::hardware;
using namespace cassio::kernel;

// -- IRQ forwarding tests --

TEST(irq_forward_delivers_to_receive_blocked_process) {
    IrqManager& irqMgr = IrqManager::getManager();
    ProcessManager& pm = ProcessManager::getManager();

    // Create a process and simulate it blocked in receive().
    Process* target = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(target != nullptr);

    ipc::Message recvBuf = {};
    target->state = ProcessState::ReceiveBlocked;
    target->msgPtr = (u32)&recvBuf;

    SyscallFrame frame = {};
    target->esp = (u32)&frame;

    // Register target for IRQ 5 (unused, no in-kernel handler).
    irqMgr.registerForward(5, target->pid);

    // Simulate IRQ 5 firing.
    irqMgr.handleIrq(0x25, 0);

    // Target should be woken with IrqNotify message.
    ASSERT(target->state == ProcessState::Ready);
    ASSERT_EQ(recvBuf.type, ipc::MessageType::IrqNotify);
    ASSERT_EQ(recvBuf.arg1, 5u);

    // Return value should be 0 (kernel PID).
    ASSERT_EQ(frame.eax, 0u);

    // Clean up.
    irqMgr.registerForward(5, 0);
    pm.destroy(target->pid);
}

TEST(irq_forward_sets_pending_when_not_receive_blocked) {
    IrqManager& irqMgr = IrqManager::getManager();
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* target = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(target != nullptr);
    target->state = ProcessState::Ready; // Not ReceiveBlocked.

    irqMgr.registerForward(5, target->pid);

    // Simulate IRQ 5 while target is busy.
    irqMgr.handleIrq(0x25, 0);

    // Target should still be Ready (not woken from anything).
    ASSERT(target->state == ProcessState::Ready);

    // Now simulate target calling receive(). Pending IRQ should be delivered.
    pm.setCurrent(target);
    ipc::Message recvBuf = {};
    i32 result = sh.receive(&recvBuf, 0, 0);

    // -2 means IRQ notification delivered.
    ASSERT_EQ(result, static_cast<i32>(-2));
    ASSERT_EQ(recvBuf.type, ipc::MessageType::IrqNotify);
    ASSERT_EQ(recvBuf.arg1, 5u);

    // A second receive() with no more pending IRQs should block.
    result = sh.receive(&recvBuf, 0, 0);
    ASSERT_EQ(result, 0);
    ASSERT(target->state == ProcessState::ReceiveBlocked);

    // Clean up.
    irqMgr.registerForward(5, 0);
    pm.destroy(target->pid);
}

TEST(irq_forward_coexists_with_in_kernel_handler) {
    IrqManager& irqMgr = IrqManager::getManager();
    ProcessManager& pm = ProcessManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    // Register a process for IRQ 0 (PIT timer) alongside the in-kernel handler.
    Process* target = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(target != nullptr);

    ipc::Message recvBuf = {};
    target->state = ProcessState::ReceiveBlocked;
    target->msgPtr = (u32)&recvBuf;

    SyscallFrame frame = {};
    target->esp = (u32)&frame;

    irqMgr.registerForward(0, target->pid);

    // Fire IRQ 0. PIT should still increment ticks AND target should get notified.
    u32 before = pit.getTicks();
    irqMgr.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    ASSERT_EQ(after, before + 1);
    ASSERT(target->state == ProcessState::Ready);
    ASSERT_EQ(recvBuf.type, ipc::MessageType::IrqNotify);
    ASSERT_EQ(recvBuf.arg1, 0u);

    // Clean up.
    irqMgr.registerForward(0, 0);
    pm.destroy(target->pid);
}

TEST(irq_register_forward_invalid_irq_fails) {
    IrqManager& irqMgr = IrqManager::getManager();

    i32 result = irqMgr.registerForward(16, 1);
    ASSERT_EQ(result, static_cast<i32>(-1));

    result = irqMgr.registerForward(255, 1);
    ASSERT_EQ(result, static_cast<i32>(-1));
}

TEST(irq_forward_irq_takes_priority_over_send_queue) {
    IrqManager& irqMgr = IrqManager::getManager();
    ProcessManager& pm = ProcessManager::getManager();
    SyscallHandler& sh = SyscallHandler::getSyscallHandler();

    Process* receiver = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    Process* sender = pm.create(0x3000, 0x4000, 0x08, 0x10, 0);
    ASSERT(receiver != nullptr);
    ASSERT(sender != nullptr);

    // Set up: sender in queue AND pending IRQ for receiver.
    sender->state = ProcessState::SendBlocked;
    sender->msg = {42, 1, 2, 3, 4, 5};
    receiver->sendQueuePush(sender->pid);

    irqMgr.registerForward(5, receiver->pid);
    receiver->state = ProcessState::Ready;
    irqMgr.handleIrq(0x25, 0); // Sets pending (receiver not ReceiveBlocked).

    // receive() should deliver the IRQ first (highest priority).
    pm.setCurrent(receiver);
    ipc::Message recvBuf = {};
    i32 result = sh.receive(&recvBuf, 0, 0);

    ASSERT_EQ(result, static_cast<i32>(-2));
    ASSERT_EQ(recvBuf.type, ipc::MessageType::IrqNotify);
    ASSERT_EQ(recvBuf.arg1, 5u);

    // Next receive() should deliver from the send queue.
    result = sh.receive(&recvBuf, 0, 0);
    ASSERT_EQ(result, static_cast<i32>(sender->pid));
    ASSERT_EQ(recvBuf.type, 42u);

    // Clean up.
    irqMgr.registerForward(5, 0);
    pm.destroy(receiver->pid);
    pm.destroy(sender->pid);
}
