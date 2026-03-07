# In-Kernel Testing Framework Design

## Summary

An in-kernel test framework for CassiOS that builds a separate test kernel, runs all tests inside QEMU, reports results over serial (COM1), and exits with a pass/fail code. Invoked via `make test`.

## Decisions

- **Output channel**: Serial port (COM1, 0x3F8). QEMU redirects serial to a file for easy parsing.
- **Build strategy**: Separate test kernel (`bin/cassio-test.bin`), not a flag in the production kernel. `kernel.cpp` is excluded from the test build; `test_kernel.cpp` provides its own `start()`.
- **Test registration**: Macro-based auto-registration via `TEST(name)`. Uses global constructors (already supported via `ctors()`) to build a linked list of tests at startup.
- **Assertions**: `ASSERT(expr)` and `ASSERT_EQ(a, b)`. Both print file/line on failure, mark the test failed, and return early.
- **Output format**: One line per test, human-readable and grep-friendly:
  ```
  [PASS] gdt_segment_offsets
  [FAIL] port_read_write: expected 0x42, got 0x00 at test_port.cpp:12
  [DONE] 5 passed, 1 failed
  ```
- **QEMU exit**: Debug exit device (`-device isa-debug-exit,iobase=0xf4,iosize=0x04`). Write 0x00 for all-pass (QEMU exits with code 1), 0x01 for any-fail (exits with code 3). Makefile translates exit code 1 to success.
- **Isolation**: None (Approach A). A crash in one test kills the run. This is acceptable -- a crash indicates a serious problem.

## File Structure

```
tests/
  test.hpp             # framework: TEST() macro, ASSERT/ASSERT_EQ, serial I/O, runner
  test_kernel.cpp      # entry point: boots, runs all tests, exits QEMU
  test_gdt.cpp         # GDT tests
  test_iostream.cpp    # iostream/VGA buffer tests
```

Test files follow the naming convention `test_<module>.cpp`.

## Framework API

### Writing a test

```cpp
// tests/test_gdt.cpp
#include "test.hpp"
#include "core/gdt.hpp"

TEST(gdt_code_segment_offset) {
    GlobalDescriptorTable gdt;
    ASSERT_EQ(gdt.codeSegmentSelector(), 0x08);
}
```

### TEST(name) macro

Expands to:
1. A test function `void test_##name()`
2. A static `TestNode` containing the name string, function pointer, and next pointer
3. A global constructor that prepends the node to a static linked list (`test_list_head`)

No heap allocation. Registration happens during `ctors()` before `start()` is called.

### Assertions

- `ASSERT(expr)` -- fails with: `[FAIL] name: assertion failed: "expr" at file:line`
- `ASSERT_EQ(a, b)` -- fails with: `[FAIL] name: expected <a>, got <b> at file:line`

Both print to serial, mark the test as failed, and `return` from the test function.

### Serial output

A minimal COM1 driver (~15 lines) using the existing `Port8Bit` class. Functions: `serial_init()`, `serial_putchar()`, `serial_puts()`, `serial_put_dec()`.

## Test Runner

```cpp
void start(void* multiboot, u32 magic) {
    serial_init();

    u32 passed = 0, failed = 0;
    for (TestNode* t = test_list_head; t; t = t->next) {
        TestResult result = run_test(t);
        if (result == PASS) {
            serial_puts("[PASS] ");
            serial_puts(t->name);
            serial_puts("\n");
            passed++;
        } else {
            failed++;  // FAIL line already printed by ASSERT
        }
    }

    serial_puts("[DONE] ");
    serial_put_dec(passed);
    serial_puts(" passed, ");
    serial_put_dec(failed);
    serial_puts(" failed\n");

    // Exit QEMU: 0x00 -> exit code 1 (pass), 0x01 -> exit code 3 (fail)
    Port8Bit debug_exit(0xf4);
    debug_exit.write(failed > 0 ? 0x01 : 0x00);
}
```

## Makefile

A `test` target that builds the test kernel and runs it headless:

```makefile
test: bin/cassio-test.bin
	@qemu-system-i386 -machine pc -kernel bin/cassio-test.bin \
	    -display none -serial file:/tmp/cassio-test-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -no-reboot -net none; \
	EXIT_CODE=$$?; \
	cat /tmp/cassio-test-results.txt; \
	[ $$EXIT_CODE -eq 1 ]
```

The test binary is linked from all `tests/test_*.cpp` files plus all `src/` object files except `kernel.o`.

## Initial Test Scope

Phase 1 (this implementation):
- **GDT**: segment selector offsets (relevant to issue #9)
- **iostream**: VGA buffer writes produce correct character/attribute bytes
- **Port**: construction and port number storage

Phase 2 (future):
- Interrupts, keyboard, mouse (require GDT + interrupt manager setup)
