# Syscall Interface Design

## Overview

Implement `int 0x80` syscall dispatch with a small set of syscalls while still running in a flat kernel (no userspace). This separates the syscall plumbing from the complexity of ring 3 transitions and scheduling introduced in Phase 7. Design decisions here (calling convention, table layout, handler structure) carry forward to all later phases.

## Calling Convention

Linux i386 convention:

- Syscall number in EAX
- Arguments in EBX, ECX, EDX (up to 3 args; ESI, EDI, EBP available if needed later)
- Return value in EAX
- `int 0x80` triggers the syscall

## Syscall Table

Syscall numbers defined as named constexpr constants in a `SyscallNumber` namespace (consistent with `AtaCommand`, `KeyboardCommand`, etc.):

```cpp
namespace SyscallNumber {
    constexpr u32 Write  = 0;
    constexpr u32 Read   = 1;
    constexpr u32 Sleep  = 2;
    constexpr u32 Uptime = 3;
    constexpr u32 Count  = 4;
}
```

## Syscalls

| Number | Name | Args | Returns | Description |
|--------|------|------|---------|-------------|
| 0 | write | fd (EBX), buf (ECX), len (EDX) | bytes written, or -1 | fd=1: VGA terminal, fd=2: COM1 serial |
| 1 | read | fd (EBX), buf (ECX), len (EDX) | bytes read, or -1 | fd=0: keyboard buffer. Non-blocking: returns 0 if no input available |
| 2 | sleep | ms (EBX) | 0 | Spin-waits on PIT ticks for the given duration |
| 3 | uptime | (none) | tick count | Returns PitTimer::getTicks() |

Invalid syscall numbers return -1.

## Architecture

### Approach

Extend InterruptManager with a `setGate()` method for registering non-IRQ interrupt gates (Approach B). This avoids duplicating IDT logic while keeping the syscall handler self-contained. A full interrupt subsystem refactor (splitting InterruptManager into IDT owner + ExceptionHandler + IrqManager + SyscallHandler) is deferred to a later phase when exception handlers and more vectors justify it.

### InterruptManager Changes

New public method:

```cpp
void setGate(u8 vector, void(*handler)(), u8 dpl);
```

Wraps `setInterrupt()` with the code segment offset and interrupt gate flags, but lets the caller specify the DPL. SyscallHandler uses this to register a DPL=3 gate at vector 0x80.

### Assembly Stub

`src/core/syscall_stub.s` -- standalone assembly entry point, separate from the hardware IRQ stubs in `stub.s`:

1. Export `syscallEntry` symbol
2. Save registers (`pusha`, segment registers)
3. Pass EAX (syscall number), EBX, ECX, EDX to the `handleSyscall` C-linkage function
4. Place return value into EAX's slot in the saved register frame
5. Restore registers, `iret`
6. No PIC EOI -- software interrupt, not hardware IRQ

### SyscallHandler Class

`include/core/syscall.hpp` / `src/core/syscall.cpp` -- singleton in `core/`, following the same pattern as InterruptManager.

```cpp
class SyscallHandler {
public:
    static SyscallHandler& getSyscallHandler();

    void load();
    i32 handleSyscall(u32 number, u32 ebx, u32 ecx, u32 edx);

    SyscallHandler(const SyscallHandler&) = delete;
    SyscallHandler(SyscallHandler&&) = delete;
    SyscallHandler& operator=(const SyscallHandler&) = delete;
    SyscallHandler& operator=(SyscallHandler&&) = delete;

private:
    SyscallHandler();

    i32 write(u32 fd, u32 buf, u32 len);
    i32 read(u32 fd, u32 buf, u32 len);
    i32 sleep(u32 ms);
    i32 uptime();
};
```

- `load()` calls `InterruptManager::setGate(0x80, &syscallEntry, 3)`
- `handleSyscall()` validates the number against `SyscallNumber::Count`, switches to the appropriate private method, returns -1 for unknown numbers

The `extern "C" handleSyscall` free function bridges from assembly to the class, same pattern as `handleInterrupt` in InterruptManager:

```cpp
extern "C" i32 handleSyscall(u32 eax, u32 ebx, u32 ecx, u32 edx);
```

### Kernel Integration

Called from `start()` after `im.load(gdt)`:

```cpp
SyscallHandler& sh = SyscallHandler::getSyscallHandler();
sh.load();
```

### Keyboard Ring Buffer

KeyboardDriver gets a fixed-size ring buffer. Keystrokes are appended to the buffer in the interrupt handler. `SyscallHandler::read()` drains the buffer into the caller's buffer. Non-blocking: returns 0 if empty.

## Testing

### Automated (`make test`)

Inline `int 0x80` from ring 0 -- tests the dispatch path and syscall logic, but not the DPL=3 privilege transition (no userspace yet):

- **uptime**: call twice, verify second >= first
- **sleep**: call with small value, verify ticks advanced
- **write**: fd=1 with string buffer, verify return equals length; invalid fd returns -1
- **read**: no pending input returns 0; invalid fd returns -1
- **invalid number**: syscall number >= Count returns -1

### Manual (`syscall` shell command)

A `syscall` shell command for manual testing: `syscall <number> [arg1] [arg2] [arg3]`.

- For `write` (syscall 0), remaining arguments after the fd are treated as a string, and the command passes the buffer address and length to the syscall.
- For other syscalls, arguments are parsed as integers.
- Prints the return value.

This command is a temporary testing tool that will be removed when userspace arrives.

## File Locations

| File | Purpose |
|------|---------|
| `include/core/syscall.hpp` | SyscallHandler class and SyscallNumber namespace |
| `src/core/syscall.cpp` | SyscallHandler implementation |
| `src/core/syscall_stub.s` | Assembly entry point for int 0x80 |
| `tests/core/test_syscall.cpp` | Automated syscall tests |
