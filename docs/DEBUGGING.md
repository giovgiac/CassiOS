# Debugging Methodology

A principled approach to fixing bugs and issues. Follow these four steps in order, and loop until the fix is verified.

## 1. Reproduce

Before investigating, reproduce the issue reliably.

- Write a minimal script or test that demonstrates the problem.
- Capture concrete output: exact values, positions, error messages, raw data.
- Avoid guessing at the cause -- let the reproduction guide diagnosis.

If the issue cannot be reproduced, gather more information before proceeding.

## 2. Diagnose

Examine the reproduction output to identify the root cause.

- **Isolate first, theorize second.** Comment out code, add/remove components one at a time, and re-run to narrow the scope. Do this BEFORE reading disassembly, inspecting memory dumps, or forming theories about what might be wrong. The fastest path to a root cause is binary search through the code, not deep analysis of a single hypothesis.
- Compare actual output against expected output in detail.
- Inspect raw data (e.g., VGA memory bytes, register values, interrupt logs) rather than relying on visual inspection alone.
- Question assumptions -- the first hypothesis is often wrong. Example: garbled VGA text that looked like a string corruption bug turned out to be leftover BIOS text that the kernel never cleared. Example: a keyboard driver that appeared broken was actually hitting a GPF caused by a GDT layout issue completely unrelated to the driver code.
- Do not get distracted by side issues. If you notice something suspicious but unrelated to the bug you're tracking, note it and move on. Fix one bug at a time.

## 3. Fix

Apply the minimal change that addresses the root cause.

- Fix the actual cause, not the symptoms.
- Keep the change focused -- avoid unrelated refactoring.
- Consider whether the fix introduces secondary issues.

## 4. Verify

Re-run the reproduction and confirm the issue is resolved. **This step is not optional and must not be skipped.**

- Run the exact reproduction from step 1 and confirm the output is now correct.
- Test edge cases and alternate scenarios.
- **If the issue is still reproducible or regressions are found, loop back to step 2.** Continue iterating until the fix is proven and no regressions exist.

## Summary

```
Reproduce -> Diagnose -> Fix -> Verify
    ^                             |
    |_____________________________|
         (loop until verified)
```

Do not consider a bug fixed until you have proven it through the verify step. A fix that has not been verified is not a fix.

---

## OS-Specific Debugging Techniques

### QEMU as the test environment

Run the kernel with QEMU instead of VirtualBox for development:

```
qemu-system-i386 -machine pc -kernel bin/cassio.bin -net none
```

Key flags:
- `-machine pc` -- required; without it QEMU may not load the multiboot kernel
- `-net none` -- disables iPXE network boot ROM, which otherwise writes to VGA memory and corrupts kernel output
- `-no-reboot` -- halts on triple fault instead of rebooting (makes crashes visible)
- `-display none` -- headless mode for automated testing

### Capturing VGA screenshots

Since the kernel writes to the VGA text buffer at 0xB8000, use QEMU's monitor to take screenshots for automated/headless testing:

```bash
#!/bin/bash
# qemu-test.sh [name] [sendkeys...]
# Takes a screenshot of VGA output, optionally after sending keystrokes.
set -e
SOCK=/tmp/cassio-test/mon.sock
NAME=${1:-test}; shift 2>/dev/null || true
mkdir -p /tmp/cassio-test
pkill -f qemu-system-i386 2>/dev/null || true
sleep 0.5; rm -f "$SOCK"

qemu-system-i386 -machine pc -kernel bin/cassio.bin \
    -display none -monitor "unix:$SOCK,server,nowait" \
    -no-reboot -net none -daemonize
sleep 2

for key in "$@"; do
    printf 'sendkey %s\n' "$key" | socat - "UNIX-CONNECT:$SOCK" >/dev/null 2>&1
    sleep 0.3
done
[ $# -gt 0 ] && sleep 1

printf 'screendump /tmp/cassio-test/%s.ppm\n' "$NAME" \
    | socat - "UNIX-CONNECT:$SOCK" >/dev/null 2>&1
sleep 1
ffmpeg -y -i "/tmp/cassio-test/${NAME}.ppm" \
    -update 1 "/tmp/cassio-test/${NAME}.png" 2>/dev/null
printf 'quit\n' | socat - "UNIX-CONNECT:$SOCK" >/dev/null 2>&1
echo "/tmp/cassio-test/${NAME}.png"
```

Usage: `./qemu-test.sh boot` or `./qemu-test.sh typing h e l l o`

### Dumping raw VGA memory

Use the QEMU monitor to read VGA memory directly (each character is 2 bytes: char + attribute):

```
printf 'xp /160xb 0xb8000\n' | socat - "UNIX-CONNECT:$SOCK"
```

This dumps the first row (80 chars x 2 bytes). Useful for distinguishing between rendering bugs and data bugs -- you can see exact character values and attributes.

### Inspecting CPU state

Check registers via the monitor:

```
printf 'info registers\n' | socat - "UNIX-CONNECT:$SOCK"
```

Key things to look for:
- **ESP in the wrong range** -- if ESP points into .text or data sections instead of the stack (BSS), the stack has been corrupted
- **EIP in a non-code section** -- if EIP is in .data or vtable area, the CPU is executing data (crash)
- **Segment registers** -- after loading a new GDT, check that CS/DS/SS use valid selectors. A mismatch between the selector value and the new GDT layout causes GPFs on the next interrupt return

### Interrupt and exception tracing

Run with `-d int -D /tmp/int_log.txt` to log all interrupts and exceptions:

```
qemu-system-i386 -machine pc -kernel bin/cassio.bin -net none \
    -no-reboot -display none -d int -D /tmp/int_log.txt
```

Then search for exceptions: `grep "check_exception" /tmp/int_log.txt | head`

- `new 0xd` = General Protection Fault (check the error code for the faulting selector)
- `new 0xe` = Page Fault
- `new 0x8` = Double Fault (usually means the first exception handler also faulted)

### Instruction tracing

For tracing which instructions execute (useful to confirm the kernel entry point is reached):

```
qemu-system-i386 -machine pc -kernel bin/cassio.bin -net none \
    -no-reboot -display none -d in_asm -D /tmp/asm_log.txt
```

Then check: `grep "0x0010" /tmp/asm_log.txt | head` to see if kernel-space code executes.

### Isolation by elimination

The single most effective technique: **comment out code and re-test.**

- System hangs after `im.activate()`? Comment out driver loading and test again. Still hangs? The problem is in the interrupt infrastructure. Works now? Add drivers back one at a time.
- VGA output is garbled? Comment out all printing except one line. Still garbled? The issue is in VGA memory state, not the print logic.
- Keyboard not working? Check if the interrupt fires at all (interrupt log), then check if the handler runs (add a VGA write in the handler).

This is faster and more reliable than reading disassembly or forming theories. **Always narrow the scope before diving deep.**

### Binary layout sensitivity

Adding or removing object files from the link changes the binary layout (section offsets shift). This can cause subtle visual bugs even if the new code is never called.

Example: linking an unused `serial.o` shifted the binary enough to zero out VGA attribute bytes, making text black-on-black (invisible). The fix was to not link `serial.o` into the production kernel.

When debugging display issues after adding new code, check whether the issue persists with the new object file removed from the link. Use `objdump -h bin/cassio.bin` to compare section layouts before and after.

### Comparing binary layouts

Use `objdump -h` to inspect section addresses and sizes:

```
objdump -h bin/cassio.bin | grep -E "\.text|\.data|\.bss"
```

Key things to check:
- `.bss` start and end addresses (start + size) -- ensure they don't overlap VGA (0xB8000) or other memory-mapped regions
- Whether adding a new object file shifts `.data` or `.bss` in unexpected ways
