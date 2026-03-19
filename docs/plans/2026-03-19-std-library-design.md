# Standard Library Redesign

Replace the current `common/` (libcommon) and `userspace/libs/libcassio/` (libcassio) with a modular standard library under `libs/` at the project root. Each module builds to its own static library, uses the `std` namespace, and can be independently linked by kernel or userspace code.

## Directory Layout

Each module follows this structure:

```
libs/<module>/
  include/std/<module>.hpp
  src/<module>.cpp
  tests/test_<module>.cpp
  Makefile
```

Each builds to `lib/libstd_<module>.a`. Header-only modules omit `src/`.

## Namespace Conventions

All public API lives under `std::<module>`:

```cpp
namespace std {
namespace fmt {
    // ...
}
}
```

Exception: `std::types` re-exports its types into `std::` directly (e.g., `std::u32`) since they are used everywhere. The test framework keeps the `test::` namespace.

## Include Conventions

Headers are included as `#include <std/module.hpp>`. Complex modules may have sub-headers (e.g., `#include <std/vga/color.hpp>`) with the main header re-exporting everything for typical use.

Include guards follow the pattern `STD_<MODULE>_HPP` (or `STD_<MODULE>_<FILE>_HPP` for sub-headers).

## Module Inventory

### Core (kernel + userspace)

| Module | Lib | Depends on | Contents |
|--------|-----|-----------|----------|
| types | libstd_types.a | *(none)* | u8, u16, u32, u64, i8-i64, f32, f64, usize, isize |
| mem | libstd_mem.a | types | mem::copy, mem::move, mem::set, mem::compare |
| str | libstd_str.a | types | str::eq, str::copy, str::len, str::to_u32 |
| fmt | libstd_fmt.a | types, str | sprintf-like string formatting (new) |
| heap | libstd_heap.a | types, mem | HeapAllocator, operator new/delete, userspace sbrk grow |
| collections | *(header-only)* | types | collections::LinkedList\<T\> (at `std/collections/list.hpp`) |
| io | *(header-only)* | types | io::Port\<T\>, io::PortType enum (at `std/io.hpp`) |
| msg | libstd_msg.a | types | Message struct, MessageType constants |
| test | libstd_test.a | types, str | TEST/ASSERT macros, test runner (uses `test::` namespace) |

### Userspace only

| Module | Lib | Depends on | Contents |
|--------|-----|-----------|----------|
| syscall | libstd_syscall.a | types | SyscallNumber constants, System class (write, sleep, uptime, etc.) |
| ipc | libstd_ipc.a | types, msg, syscall | IPC::send/receive/reply/notify |
| ns | libstd_ns.a | types, ipc | Nameserver client (lookup, register) |
| vga | libstd_vga.a | types, ipc, ns, msg | Vga instance class, auto-resolves PID |
| vfs | libstd_vfs.a | types, ipc, ns, msg | Vfs instance class, auto-resolves PID |
| ata | libstd_ata.a | types, ipc, ns, msg | Ata instance class, auto-resolves PID |
| kbd | libstd_kbd.a | types, ipc, ns, msg | Kbd instance class, auto-resolves PID (new) |
| mouse | libstd_mouse.a | types, ipc, ns, msg | Mouse instance class, auto-resolves PID (new) |

## Service Client Redesign

Current service clients are static-method-only classes requiring callers to pass PIDs manually. The new design makes them instance classes that auto-resolve their PID from the nameserver:

```cpp
// libs/vga/include/std/vga.hpp
#ifndef STD_VGA_HPP
#define STD_VGA_HPP

#include <std/types.hpp>
#include <std/ns.hpp>

namespace std {
namespace vga {

class Vga {
public:
    Vga();  // calls Nameserver::lookup("vga"), stores pid

    void putchar(char c);
    void write(const char* str, u32 len);
    void clear();
    void setCursor(u32 x, u32 y);
    void getCursor(u32& x, u32& y);

private:
    u32 pid;
};

}
}

#endif
```

All service clients (Vga, Vfs, Ata, Kbd, Mouse) follow this pattern. The constructor calls `Nameserver::lookup()`, which blocks until the service is registered -- this naturally handles the "wait for service" startup ordering.

The Ns (nameserver) module stays static since it always targets PID 1 (hardcoded).

## Build Integration

Each module's Makefile compiles to `lib/libstd_<module>.a`. The root Makefile gains a target per module.

Consumer include flags use `-I$(ROOT)/libs/<module>/include/` for each needed module. A shared `libs/common.mk` can collect all module include paths to avoid repetition.

Consumer link lines list only the modules they need:

```makefile
# Kernel
$(KERNEL): $(objects) lib/libstd_types.a lib/libstd_mem.a lib/libstd_heap.a ...

# Userspace service (e.g., shell)
$(SHELL_ELF): $(objects) lib/libstd_types.a lib/libstd_ipc.a lib/libstd_vga.a lib/libstd_vfs.a ...
```

## Migration Strategy

Incremental module-by-module migration (Approach B). Each PR creates one module, moves the code, updates all consumers, and removes the old source. Migration follows dependency order:

1. `types`
2. `mem`
3. `str`
4. `collections`
5. `io`
6. `msg`
7. `heap`
8. `fmt`
9. `test`
10. `syscall`
11. `ipc`
12. `ns`
13. `vga`
14. `vfs`
15. `ata`
16. `kbd`
17. `mouse`
18. Cleanup (delete empty `common/` and `userspace/libs/`)

Each PR:
1. Creates `libs/<module>/` with include/src/tests/Makefile
2. Moves or writes the code under the `std` namespace
3. Updates all consumers to use `#include <std/module.hpp>` and `std::` namespace
4. Removes the old source from `common/` or `libcassio/`
5. Updates root Makefile and consumer link lines
6. All tests pass
