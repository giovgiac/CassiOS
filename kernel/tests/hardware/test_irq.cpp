#include <hardware/irq.hpp>
#include <core/process.hpp>
#include <core/syscall.hpp>
#include <hardware/pit.hpp>
#include <test.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::hardware;
using namespace cassio::kernel;

TEST(irq_dispatch_to_pit) {
    // handleIrq(0x20) should dispatch to PitTimer and increment ticks.
    IrqManager& irq = IrqManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    u32 before = pit.getTicks();
    irq.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    ASSERT_EQ(after, before + 1);
}

TEST(irq_unregister_stops_dispatch) {
    IrqManager& irq = IrqManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    // Unregister PitTimer from IRQ 0.
    irq.unregisterHandler(0);

    u32 before = pit.getTicks();
    irq.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    // Ticks should NOT have incremented.
    ASSERT_EQ(after, before);

    // Re-register so other tests are unaffected.
    irq.registerHandler(0, &PitTimer::irqHandler);
}

TEST(irq_register_restores_dispatch) {
    IrqManager& irq = IrqManager::getManager();
    PitTimer& pit = PitTimer::getTimer();

    // Unregister, re-register, then verify dispatch works.
    irq.unregisterHandler(0);
    irq.registerHandler(0, &PitTimer::irqHandler);

    u32 before = pit.getTicks();
    irq.handleIrq(0x20, 0);
    u32 after = pit.getTicks();

    ASSERT_EQ(after, before + 1);
}

TEST(irq_handleirq_returns_esp_when_no_handler) {
    IrqManager& irq = IrqManager::getManager();

    // Vector 0x25 (IRQ 5) has no handler registered.
    u32 esp = 0x12345678;
    u32 result = irq.handleIrq(0x25, esp);

    // Should return esp unchanged.
    ASSERT_EQ(result, esp);
}

TEST(irq_idt_entries_are_distinct) {
    // Each registered IRQ vector should have a unique handler address.
    struct __attribute__((packed)) { u16 limit; u32 base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));

    u8 vectors[] = { 0x20, 0x21, 0x2C, 0x2E };
    u32 addrs[4];

    for (u32 i = 0; i < 4; ++i) {
        u8* entry = reinterpret_cast<u8*>(idtr.base) + vectors[i] * 8;
        addrs[i] = static_cast<u32>(*reinterpret_cast<u16*>(entry + 6)) << 16
                 | static_cast<u32>(*reinterpret_cast<u16*>(entry));
        ASSERT(addrs[i] != 0);
    }

    // All four should be distinct from each other.
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = i + 1; j < 4; ++j) {
            ASSERT(addrs[i] != addrs[j]);
        }
    }
}

// -- IRQ forwarding tests --

TEST(irq_forward_delivers_to_receive_blocked_process) {
    IrqManager& irqMgr = IrqManager::getManager();
    ProcessManager& pm = ProcessManager::getManager();

    // Create a process and simulate it blocked in receive().
    Process* target = pm.create(0x1000, 0x2000, 0x08, 0x10, 0);
    ASSERT(target != nullptr);

    msg::Message recvBuf = {};
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
    ASSERT_EQ(recvBuf.type, msg::MessageType::IrqNotify);
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
    target->state = ProcessState::Ready;  // Not ReceiveBlocked.

    irqMgr.registerForward(5, target->pid);

    // Simulate IRQ 5 while target is busy.
    irqMgr.handleIrq(0x25, 0);

    // Target should still be Ready (not woken from anything).
    ASSERT(target->state == ProcessState::Ready);

    // Now simulate target calling receive(). Pending IRQ should be delivered.
    pm.setCurrent(target);
    msg::Message recvBuf = {};
    i32 result = sh.receive(&recvBuf, 0, 0);

    // -2 means IRQ notification delivered.
    ASSERT_EQ(result, static_cast<i32>(-2));
    ASSERT_EQ(recvBuf.type, msg::MessageType::IrqNotify);
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

    msg::Message recvBuf = {};
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
    ASSERT_EQ(recvBuf.type, msg::MessageType::IrqNotify);
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
    sender->msg = { 42, 1, 2, 3, 4, 5 };
    receiver->sendQueuePush(sender->pid);

    irqMgr.registerForward(5, receiver->pid);
    receiver->state = ProcessState::Ready;
    irqMgr.handleIrq(0x25, 0);  // Sets pending (receiver not ReceiveBlocked).

    // receive() should deliver the IRQ first (highest priority).
    pm.setCurrent(receiver);
    msg::Message recvBuf = {};
    i32 result = sh.receive(&recvBuf, 0, 0);

    ASSERT_EQ(result, static_cast<i32>(-2));
    ASSERT_EQ(recvBuf.type, msg::MessageType::IrqNotify);
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
