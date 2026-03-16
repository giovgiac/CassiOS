# Phase 8: IPC and Microkernel Transition Design

## Overview

Migrate from monolithic to microkernel architecture. The kernel shrinks to: IPC, scheduling, memory management, and interrupt routing. Everything else (drivers, filesystem, terminal, shell) moves to userspace services communicating via synchronous message passing.

## 1. Interrupt Subsystem Refactor

Split the current `InterruptManager` into four classes with distinct responsibilities.

### InterruptManager (slimmed down)

Pure IDT management:

- Owns `GateDescriptor idt[256]`
- `setGate(u8 vector, void(*handler)(), u8 dpl)` -- writes an IDT entry
- `load()` -- fills all 256 entries with a default ignore stub, then calls `ExceptionHandler::load()`, `IrqManager::load()`, and `SyscallHandler::load()` so each registers its own vectors. Executes `lidt`.
- No PIC logic, no driver dispatch, no EOI.

### ExceptionHandler (new)

CPU exceptions, vectors 0-31:

- Singleton. `load()` sets IDT gates for vectors 0 (div-by-zero), 6 (invalid opcode), 13 (GPF), 14 (page fault).
- `handle(u8 vector, u32 esp)` -- logs the fault (vector, EIP, error code) to serial, terminates the faulting process via `ProcessManager::destroy()`, and reschedules.
- User process exceptions are fatal. Kernel exceptions trigger a panic/halt.

### IrqManager (new)

Hardware IRQs, vectors 0x20-0x2F:

- Singleton. Owns PIC initialization (remapping master to 0x20, slave to 0x28) and EOI logic.
- `load()` sets IDT gates for the IRQ vectors and initializes the PICs.
- Dispatch table: `Driver* drivers[16]` for in-kernel drivers (used during boot before userspace services are up) and `u32 forwardPid[16]` for IRQ-to-userspace forwarding.
- `handleIrq(u8 irq, u32 esp)` -- if a userspace PID is registered, sends an IPC notification; otherwise dispatches to the in-kernel driver. Sends EOI.
- `registerForward(u8 irq, u32 pid)` -- syscall-accessible method for userspace drivers to claim an IRQ.

### SyscallHandler (exists, grows)

Vector 0x80:

- Gains the new IPC syscalls: `send`, `receive`, `reply`.
- Retains `write` (fd=2 serial only), `sleep`, `uptime`. Adds `reboot`, `shutdown`, `irq_register`.
- `write` (fd=1 VGA) and `read` (fd=0 keyboard) are dropped once the corresponding services are up.

### Assembly Stubs

The assembly stubs (`stub.s`) stay as a single entry point. The top-level C dispatcher routes by vector range:

```
if (vector < 32)        -> ExceptionHandler::handle()
else if (vector < 48)   -> IrqManager::handleIrq()
else if (vector == 0x80) -> SyscallHandler::handle()
else                    -> ignore
```

## 2. IPC Mechanism

Synchronous send/receive/reply (L4/QNX style). No kernel-side message buffering.

### Syscalls

- `send(pid, msg)` -- send a message to a process, block until it receives and replies
- `receive(msg)` -- block until any process sends a message; returns the sender's PID
- `reply(pid, msg)` -- send a reply to a blocked sender, unblocking it

### Message Format

Fixed struct of 6 `u32` words, passed via pointer in EBX:

```cpp
struct Message {
    u32 type;    // message type / opcode
    u32 arg1;    // argument 1
    u32 arg2;    // argument 2
    u32 arg3;    // argument 3
    u32 arg4;    // argument 4
    u32 arg5;    // argument 5
};
```

Register-only for this phase (no buffer copy between address spaces). Buffer copy extension added later when needed (prerequisite for Phase 9 FAT32 bulk I/O).

### Process States

```cpp
enum class ProcessState : u8 {
    Empty,
    Ready,
    Running,
    SendBlocked,    // blocked in send(), waiting for receiver to receive+reply
    ReceiveBlocked  // blocked in receive(), waiting for a sender
};
```

### Rendezvous Logic

- `send(pid, msg)`: if the target is `ReceiveBlocked`, copy message to target, wake target (-> Ready), caller becomes `SendBlocked` (waiting for reply). If target is not `ReceiveBlocked`, caller joins the target's send queue and becomes `SendBlocked`.
- `receive(msg)`: if the caller's send queue is non-empty, dequeue the first sender, copy its message, return sender PID. If empty, caller becomes `ReceiveBlocked`.
- `reply(pid, msg)`: find the `SendBlocked` process, copy reply message to it, wake it (-> Ready).

### Send Queue

Each process has a fixed array of 15 PID slots (max processes - 1) for pending senders. FIFO ordering.

### Scheduler Integration

`SendBlocked` and `ReceiveBlocked` processes are skipped by the round-robin scheduler. Only `Ready` and `Running` are schedulable.

## 3. IRQ Forwarding

When a hardware IRQ fires and a userspace driver has registered for it, the kernel delivers it via IPC.

### Mechanism

- A userspace driver calls `receive()` in a loop, waiting for work.
- When an IRQ fires, `IrqManager::handleIrq()` checks `forwardPid[irq]`. If the target is `ReceiveBlocked`, deliver a message with `type = MSG_IRQ_NOTIFY` and `arg1 = irq number`. The driver wakes, handles the hardware (port I/O with IOPL=3), and loops back to `receive()`.
- If the driver is not `ReceiveBlocked` (still processing), the kernel sets a pending flag for that IRQ. Next time the driver calls `receive()`, it immediately gets the pending notification instead of blocking.
- One pending flag per IRQ is sufficient -- if multiple IRQs fire while the driver is busy, it just needs to know "there's work to do" and re-check the hardware.

### Registration

New syscall `sys_irq_register(u8 irq)` calls `IrqManager::registerForward(irq, callerPid)`. One process per IRQ.

### EOI

The kernel sends PIC EOI before forwarding to userspace. The IRQ is acknowledged at the hardware level immediately.

### IRQ vs Regular IPC

The driver's `receive()` handles both IRQ notifications and client requests. Distinguished by `msg.type`: `MSG_IRQ_NOTIFY` for hardware, application-defined types for client requests.

## 4. Nameserver

First userspace process, well-known PID 1. Provides name-to-PID lookup for service discovery.

### Protocol

- **Register**: `type = MSG_NS_REGISTER`, `arg1`-`arg4` = name (up to 16 chars packed). Reply is acknowledgement.
- **Lookup**: `type = MSG_NS_LOOKUP`, `arg1`-`arg4` = name. Reply: `arg1` = PID of registered service, or 0 if not found.

### Internal State

Fixed table of 16 entries (name string + PID). Linear scan for lookup.

## 5. Service Architecture

Each service is a standalone userspace ELF binary loaded from a multiboot module. All follow the same pattern: register with nameserver, loop on `receive()`.

### Keyboard Service (`userspace/kbd/`)

- Registers as "kbd" with nameserver.
- `sys_irq_register(1)` to claim IRQ 1.
- Main loop: `receive()` -- `MSG_IRQ_NOTIFY` triggers port 0x60 read and scancode translation, stored in ring buffer. `MSG_KBD_READ` from clients pops from buffer and replies.
- IOPL=3 for port 0x60/0x64.

### Mouse Service (`userspace/mouse/`)

- Registers as "mouse" with nameserver.
- `sys_irq_register(12)` to claim IRQ 12.
- IRQ triggers port 0x60 reads, client requests get mouse state via IPC.
- IOPL=3 for port access.

### ATA Service (`userspace/ata/`)

- Registers as "ata" with nameserver.
- `sys_irq_register(14)` to claim IRQ 14.
- Handles `MSG_ATA_READ` and `MSG_ATA_WRITE` from clients. Bulk sector data transfer deferred until buffer IPC is added (prerequisite for Phase 9). For this phase, ATA migrates to userspace and handles IRQs, but full read/write of sector data is limited to register-sized payloads.
- IOPL=3 for ATA ports.

### VGA Terminal Service (`userspace/vga/`)

- Registers as "vga" with nameserver.
- Owns VGA memory (kernel maps 0xB8000 into this process's address space).
- Handles `MSG_VGA_WRITE` (char, x, y, color), `MSG_VGA_PUTSTR` (short strings packed across message words), `MSG_VGA_CLEAR`, `MSG_VGA_SETCURSOR`, `MSG_VGA_SCROLL`.
- No IRQ -- purely request-driven.
- IOPL=3 for cursor CRTC ports (0x3D4/0x3D5).

### Filesystem Service (`userspace/vfs/`)

- Registers as "vfs" with nameserver.
- Contains the in-memory filesystem logic (moved from kernel).
- Handles `MSG_VFS_OPEN`, `MSG_VFS_READ`, `MSG_VFS_WRITE`, `MSG_VFS_MKDIR`, `MSG_VFS_REMOVE`, `MSG_VFS_LIST`, etc.
- No IRQ -- purely request-driven.
- File data in register-only messages limited to ~20 bytes per message. Acceptable for this phase (small files via shell commands). Improves with buffer IPC.

### Shell (`userspace/shell/`)

- On startup, looks up "kbd", "vga", "vfs" via nameserver.
- Main loop: sends `MSG_KBD_READ` to keyboard service for input, `MSG_VGA_WRITE` to VGA service for output. On enter, parses command and dispatches to appropriate service.
- `reboot`, `shutdown` use kernel syscalls directly.
- `uptime`, `sleep` remain kernel syscalls.

## 6. Port I/O from Userspace

Driver processes (kbd, mouse, ata, vga) get IOPL=3 set in their initial EFLAGS when the kernel creates the process. This allows all port I/O instructions (in/out) from ring 3. Simple and sufficient for a small set of trusted driver processes.

Per-port isolation via I/O permission bitmap in the TSS can be added later without changing driver code (just stop setting IOPL=3 and configure the bitmap instead).

## 7. Boot Sequence

### GRUB Multiboot Modules

Each userspace binary is a multiboot module, loaded by GRUB:

```
module 0: nameserver
module 1: kbd
module 2: mouse
module 3: ata
module 4: vga
module 5: vfs
module 6: shell
```

### Kernel Startup (revised `start()`)

1. Initialize GDT (kernel + user segments + TSS)
2. Initialize interrupt subsystem: `InterruptManager::load()` calls `ExceptionHandler::load()`, `IrqManager::load()`, `SyscallHandler::load()`
3. Initialize memory: PhysicalMemoryManager, KernelHeap, PagingManager
4. Initialize PIT timer (stays in-kernel for scheduling)
5. Initialize Scheduler
6. Register kernel task (PID 0)
7. For each multiboot module 0-6: `ElfLoader::load()`, create process, allocate stacks, add to scheduler. Set IOPL=3 in EFLAGS for driver processes.
8. Enable interrupts
9. Kernel idle loop: `for (;;) hlt;`

The kernel no longer runs the shell directly. After spawning all processes, it idles and handles interrupts/syscalls/IPC.

### Service Startup Ordering

The shell retries `lookup()` if a service hasn't registered yet (returns PID 0). No kernel-side dependency management needed.

## 8. Syscall Table

| Number | Name | Purpose |
|--------|------|---------|
| 0 | send | IPC send (blocks until reply) |
| 1 | receive | IPC receive (blocks until message) |
| 2 | reply | IPC reply (unblocks sender) |
| 3 | irq_register | Claim an IRQ for userspace forwarding |
| 4 | write | Serial/COM1 debug output (fd=2 only) |
| 5 | sleep | Timer delay (ms) |
| 6 | uptime | Return tick counter |
| 7 | reboot | Kernel writes 0xFE to port 0x64 |
| 8 | shutdown | Kernel does cli + hlt |

Previous syscall numbers are reassigned. The old `read` (fd=0 keyboard) is removed.

## 9. What Gets Removed from the Kernel

- `Shell` (core/shell.cpp, core/commands/)
- `KeyboardDriver` (drivers/keyboard.cpp)
- `MouseDriver` (drivers/mouse.cpp)
- `AtaPioDriver` (drivers/ata.cpp)
- `VgaTerminal` (hardware/terminal.cpp)
- `Filesystem` (filesystem/filesystem.cpp)
- `DriverManager` (hardware/driver.cpp) -- no more in-kernel driver registry
- `Driver` base class -- replaced by userspace service pattern

## 10. What Stays in the Kernel

- GDT / TSS
- Interrupt subsystem: InterruptManager (IDT), ExceptionHandler, IrqManager, SyscallHandler
- Memory management: PagingManager, PhysicalMemoryManager, KernelHeap
- Process management: ProcessManager, Scheduler
- PIT timer (drives scheduling)
- IPC mechanism
- ELF loader
- Serial (COM1) for debugging

## 11. Testing Strategy

Each implementation issue includes its own tests.

- **Interrupt refactor**: test exception handlers fire on forced faults (div-by-zero), test IrqManager dispatch.
- **IPC mechanism**: test send/receive/reply between kernel-created processes. Test blocking states, send queue ordering.
- **IRQ forwarding**: test registered process receives IRQ notifications. Test pending flag when driver is busy.
- **Nameserver**: test register + lookup round-trip. Test lookup of unregistered name returns 0.
- **Service migration**: for each service, run end-to-end. E.g., keyboard migration: verify keystrokes arrive at test consumer via IPC. VGA migration: verify characters on screen via QEMU screenshot.
- **Full integration**: boot with all services, type a command in the shell, verify output via QEMU screenshot.
