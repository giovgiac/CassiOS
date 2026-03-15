# Userspace and Process Management Design

## Overview

Add ring 3 execution, an ELF loader, preemptive round-robin scheduling, and per-process address spaces. A single init process loaded from a GRUB multiboot module runs alongside the existing kernel shell. The interrupt subsystem refactor is deferred to Phase 8.

## Scope

1. **GDT + TSS** -- user code/data segments, Task State Segment for ring 0 stack on privilege transitions
2. **Process management** -- Process struct, fixed-size process table, ProcessManager singleton
3. **Scheduler** -- preemptive round-robin via PIT timer, context switching by swapping saved ESP
4. **Per-process address spaces** -- separate page directories, shared kernel mappings, user pages in lower 3 GiB
5. **ELF loader** -- parse ELF32 executables from multiboot modules, load PT_LOAD segments into user address spaces
6. **Ring 3 entry** -- fake interrupt frame for initial iret to userspace, TSS.esp0 updates on context switch
7. **Init process** -- userspace program that prints uptime every 5 seconds via syscalls

Out of scope: fork/exec, multiple userspace programs, IPC, interrupt subsystem refactor (all Phase 8).

## GDT and TSS

### Current GDT layout

| Offset | Segment | Flags |
|--------|---------|-------|
| `0x00` | null | -- |
| `0x08` | kernel code | `0x9A` (ring 0, execute/read) |
| `0x10` | kernel data | `0x92` (ring 0, read/write) |

### New GDT layout

| Offset | Segment | Flags | Purpose |
|--------|---------|-------|---------|
| `0x00` | null | -- | -- |
| `0x08` | kernel code | `0x9A` | ring 0, execute/read |
| `0x10` | kernel data | `0x92` | ring 0, read/write |
| `0x18` | user code | `0xFA` | ring 3, execute/read (0x9A with DPL=3) |
| `0x20` | user data | `0xF2` | ring 3, read/write (0x92 with DPL=3) |
| `0x28` | TSS | `0x89` | 32-bit TSS descriptor |

### TSS

A 104-byte struct stored as a member of `GlobalDescriptorTable`. Only two fields are populated:

- `ss0 = 0x10` (kernel data segment) -- stack segment for ring 0
- `esp0` -- kernel stack pointer, updated by the scheduler on every context switch via `setTssEsp0()`

The TSS descriptor in the GDT is not a standard segment -- it's built manually, pointing to the TSS struct with type=0x89 (32-bit TSS, available). After `lgdt`, the TSS selector is loaded with `ltr $0x28`.

### API additions to GlobalDescriptorTable

```cpp
void setTssEsp0(u32 esp0);   // updates TSS.esp0
u16 getUserCodeOffset();       // returns 0x18
u16 getUserDataOffset();       // returns 0x20
u16 getTssOffset();            // returns 0x28
```

### Impact on existing code

The constructor adds 3 more entries (user code, user data, TSS) and issues `ltr`. The `lgdt` size adjusts automatically via `sizeof(GlobalDescriptorTable)`. Existing offsets `0x08` and `0x10` are unchanged -- no impact on interrupt stubs or segment registers.

## Process Control Block and Process Table

### Process struct

```cpp
enum class ProcessState : u8 {
    Empty,      // slot is unused
    Ready,      // runnable
    Running,    // currently on CPU
};

struct Process {
    u32 pid;
    ProcessState state;

    // Saved register state (filled by interrupt handler on preemption)
    u32 eax, ebx, ecx, edx;
    u32 esi, edi, ebp, esp;
    u32 eip;
    u32 eflags;
    u32 cs, ds;             // segment selectors (kernel vs user)

    // Memory
    u32 pageDirectory;      // physical address of this process's page directory

    // Stack
    u32 kernelEsp;          // top of kernel stack region (for TSS.esp0)
};
```

### Process table

A static array of 16 `Process` slots inside `ProcessManager`. PID 0 is reserved for the kernel task (the shell). PID 1 is the init userspace process.

### ProcessManager class

Singleton in `core/process.hpp` / `core/process.cpp`:

```cpp
class ProcessManager {
public:
    static ProcessManager& getManager();

    Process* create(u32 eip, u32 esp, u32 cs, u32 ds, u32 pageDirectory);
    void destroy(u32 pid);
    Process* current();

    static constexpr u32 MAX_PROCESSES = 16;

private:
    ProcessManager();

    Process processes[MAX_PROCESSES];
    u32 currentPid;
};
```

- `create()` finds the first `Empty` slot, fills in registers/segments/page directory, sets state to `Ready`, returns the `Process*`.
- `destroy()` sets the slot to `Empty`. Not used in Phase 7 but included so we don't have to retrofit it.
- `current()` returns `&processes[currentPid]`.

### Kernel task (PID 0)

Not created via `create()`. `processes[0]` is initialized during boot with the kernel's own segments (`cs=0x08`, `ds=0x10`), the kernel page directory, and the existing kernel stack. When the scheduler switches away, state is saved there; when it switches back, the shell resumes.

## Scheduler

### Design

A `Scheduler` singleton driven by the PIT timer (IRQ 0, ~100 Hz). Preemptive round-robin with a configurable time slice.

### Context switch mechanism

The existing interrupt stubs already support this. The flow:

1. PIT IRQ fires, `stub.s` saves registers via `pusha` + segment register pushes, calls `handleInterrupt(0x20, esp)`
2. `InterruptManager::handleInterrupt` calls `PitTimer::handleInterrupt` which calls `Scheduler::schedule(esp)`
3. `schedule(esp)` saves `esp` into `current->esp` (captures the full register frame)
4. Picks the next `Ready` process in round-robin order
5. If the next process has a different page directory, loads it into CR3
6. Updates `TSS.esp0` to the next process's `kernelEsp`
7. Returns `next->esp`
8. Back in the stub, `mov %eax, %esp` switches to the new stack, `popa` + `iret` restores the new process's state

No new assembly is needed. The existing `handleInterrupt` return-value-as-ESP mechanism is the context switch.

### Time slice

Not every tick triggers a switch. A configurable time slice (e.g., 10 ticks = ~100ms) prevents thrashing. The scheduler counts ticks and only switches when the slice expires.

### Scheduler API

```cpp
class Scheduler {
public:
    static Scheduler& getScheduler();

    void init(GlobalDescriptorTable& gdt);
    u32 schedule(u32 currentEsp);       // called from PIT handler, returns new ESP
    void addProcess(Process* process);

private:
    Scheduler();

    GlobalDescriptorTable* gdt;
    u32 tickCount;
    u32 timeSlice;          // ticks between switches (e.g., 10)
};
```

### PitTimer integration

`PitTimer::handleInterrupt` calls `Scheduler::schedule(esp)` and returns the ESP it gets back. Minimal change to the existing driver.

### First test: kernel-only tasks

Before ring 3 exists, the scheduler is tested with two kernel-mode tasks that each print a message on a timer. Both run at ring 0 with the kernel page directory. This proves context switching works in isolation before privilege transitions are involved.

## Per-Process Address Spaces

### Design

Each process gets its own page directory. The kernel half (PDEs 768-1023, covering 0xC0000000-0xFFFFFFFF) is shared -- same entries, pointing to the same kernel page tables. The user half (PDEs 0-767, covering 0x00000000-0xBFFFFFFF) is per-process.

### PagingManager additions

```cpp
u32 createAddressSpace();
void destroyAddressSpace(u32 pdPhysical);
void mapUserPage(u32 pdPhysical, u32 virtualAddr, u32 physicalAddr, u16 flags);
```

- `createAddressSpace()` allocates a frame for a new page directory, copies PDEs 768-1023 from the kernel page directory, zeroes PDEs 0-767. Returns the physical address of the new page directory.
- `mapUserPage()` maps a page in the given page directory. Accesses page directory and page tables via their kernel-mapped virtual addresses (`phys + KERNEL_VBASE`). The existing `mapPage()` stays unchanged for kernel-space mappings.
- `destroyAddressSpace()` walks PDEs 0-767, frees any allocated page tables and mapped physical frames, then frees the page directory frame. Not exercised in Phase 7 (init never exits) but included to avoid leaks.

### User virtual address layout

| Range | Purpose |
|-------|---------|
| `0x00000000 - 0x003FFFFF` | Unmapped (null deref guard) |
| `0x00400000+` | ELF load address (.text, .data, .bss) |
| `0xBFFFE000` | Guard page (unmapped, catches stack overflow) |
| `0xBFFFF000` | User stack (4 KiB page, ESP starts at `0xC0000000`) |
| `0xC0000000+` | Kernel (shared, inaccessible from ring 3) |

## ELF Loader

### Scope

Minimal ELF32 loader for statically linked, non-relocatable executables. No dynamic linking, no shared libraries, no relocations.

### Input

A multiboot module. GRUB passes the module's start/end physical addresses in the `MultibootInfo` struct. The module is the raw init ELF binary in physical memory, accessed via `phys + KERNEL_VBASE`.

### MultibootInfo changes

The existing `MultibootInfo` struct in `include/memory/multiboot.hpp` needs module list fields:

```cpp
struct MultibootModule {
    u32 mod_start;
    u32 mod_end;
    u32 string;
    u32 reserved;
};
```

Fields `mods_count` and `mods_addr` in `MultibootInfo` point to an array of `MultibootModule` descriptors.

### ELF structures

`core/elf.hpp` defines the ELF32 header and program header structs. The loader only uses:

1. **ELF header**: magic (`0x7F 'E' 'L' 'F'`), class (32-bit), type (ET_EXEC), machine (EM_386), entry point, program header offset/count
2. **Program headers**: `PT_LOAD` segments with virtual address, file offset, file size, memory size

### ElfLoader

Stateless class with a static method:

```cpp
struct ElfLoadResult {
    u32 entryPoint;
    bool success;
};

class ElfLoader {
public:
    static ElfLoadResult load(u32 pdPhysical, const u8* elfData, u32 elfSize);
};
```

`load()` does:

1. Validate ELF header (magic, class, type, machine)
2. For each `PT_LOAD` segment:
   - Allocate physical frames (ceiling of `memsz / 4096`)
   - Map them into the target page directory at the segment's virtual address with `PAGE_USER | PAGE_PRESENT | PAGE_READWRITE`
   - Copy `filesz` bytes from the ELF data (accessed via `phys + KERNEL_VBASE`)
   - Zero remaining bytes (`memsz - filesz`) for .bss
3. Return the entry point from the ELF header

### QEMU integration

The init binary is passed as a multiboot module via `-initrd`:

```
qemu-system-i386 -kernel bin/cassio.bin -initrd bin/init.elf
```

## Ring 3 Entry

### Boot sequence

After existing initialization in `start()`:

1. Parse multiboot module 0 to find the init ELF
2. Create a new address space via `PagingManager::createAddressSpace()`
3. Load the ELF via `ElfLoader::load()`
4. Allocate a physical frame for the user stack, map at `0xBFFFF000` with `PAGE_USER`
5. Create a `Process` via `ProcessManager::create()`:
   - `eip` = ELF entry point
   - `esp` = `0xC0000000` (top of user stack page)
   - `cs` = `0x18 | 3` (user code selector, RPL=3)
   - `ds` = `0x20 | 3` (user data selector, RPL=3)
   - `pageDirectory` = physical address from step 2
6. Register the process with the scheduler
7. Set up the kernel task (PID 0) with current kernel state
8. Enable interrupts, enter the shell loop as usual

### Initial ring 3 transition

The init process doesn't start immediately. The scheduler picks it up on a future timer tick. When switching to a ring 3 process for the first time, the scheduler sets up a fake interrupt frame on the kernel stack:

```
[ss]        = 0x20 | 3     (user data, RPL=3)
[esp]       = 0xC0000000   (user stack top)
[eflags]    = 0x202         (IF=1, reserved bit 1)
[cs]        = 0x18 | 3     (user code, RPL=3)
[eip]       = entry point
```

`iret` detects the CPL change (0 -> 3), loads SS:ESP from the frame, and jumps to the entry point in ring 3. For subsequent switches (process was preempted), the saved interrupt frame from preemption is already correct.

### TSS update

Before every `iret` to ring 3, the scheduler sets `TSS.esp0` so the next interrupt from ring 3 lands on the correct kernel stack. For PID 0 (kernel task), no TSS update is needed.

### Interrupt stubs

No changes needed. The stubs push/pop `ds`, `es`, `fs`, `gs`. When returning to ring 3, the saved segment register values from the interrupt frame are restored naturally.

## Init Userspace Program

### Source

`userspace/init/main.cpp` -- a single file containing:

- A small `syscall` inline assembly wrapper for `int $0x80`
- `_start` entry point that loops: write an uptime message, sleep 5 seconds, repeat

All syscalls use the established convention (EAX=number, EBX/ECX/EDX=args, return in EAX).

### Build

- Compiled with: `-m32 -ffreestanding -nostdlib -fno-exceptions -fno-rtti`
- Linked with: `-melf_i386 -T userspace/init/linker.ld`
- Linker script: entry at `_start`, `.text` at `0x00400000`
- Output: `bin/init.elf`

New Makefile target `bin/init.elf`. `make run` and `make test` pass `-initrd bin/init.elf` to QEMU.

No shared syscall library. The inline `int $0x80` wrapper lives in `main.cpp`. A shared `libsys` can be extracted in Phase 8 when multiple userspace programs exist.

## Testing

### Automated (`make test`)

**GDT/TSS:**
- User code segment offset is `0x18`
- User data segment offset is `0x20`
- TSS offset is `0x28`
- `setTssEsp0` updates the TSS field

**ProcessManager:**
- Create a process: PID assigned, state is `Ready`
- Fill all slots: next `create()` returns null
- `current()` returns expected process

**ELF loader:**
- Minimal valid ELF32 in a byte array: `load()` maps pages and returns correct entry point
- Invalid magic: `success = false`
- Wrong machine type: `success = false`

**Scheduler:**
- Single process: `schedule()` returns same ESP (no switch)
- Two processes: returns different ESPs after time slice expires

### Manual (QEMU)

Boot with `make run`:
- Shell prompt appears and responds to input (kernel task running)
- Every ~5 seconds, init prints its uptime message (ring 3 process running)
- Both interleave on screen (preemptive scheduling working)

Screenshot verification via QEMU monitor: boot, wait 10 seconds, screendump, verify both shell prompt and init output are visible.

### Not automatable

- Ring 3 transitions (test framework runs in ring 0)
- Actual preemption across processes (requires real timer over time)
- Page fault from null deref or stack overflow in userspace

Covered by the manual QEMU test.

## File Locations

| File | Purpose |
|------|---------|
| `include/core/gdt.hpp` | Modified -- TSS struct, user segments, new methods |
| `src/core/gdt.cpp` | Modified -- TSS init, `ltr`, user segment descriptors |
| `include/core/process.hpp` | New -- Process struct, ProcessState, ProcessManager |
| `src/core/process.cpp` | New -- ProcessManager implementation |
| `include/core/scheduler.hpp` | New -- Scheduler singleton |
| `src/core/scheduler.cpp` | New -- round-robin scheduling, context switch |
| `include/core/elf.hpp` | New -- ELF32 structs, ElfLoader |
| `src/core/elf.cpp` | New -- ELF loading implementation |
| `include/memory/multiboot.hpp` | Modified -- MultibootModule, module fields |
| `include/memory/paging.hpp` | Modified -- createAddressSpace, mapUserPage, destroyAddressSpace |
| `src/memory/paging.cpp` | Modified -- new methods |
| `src/drivers/pit.cpp` | Modified -- call scheduler from handleInterrupt |
| `src/core/kernel.cpp` | Modified -- init process setup, scheduler init |
| `userspace/init/main.cpp` | New -- init userspace program |
| `userspace/init/linker.ld` | New -- linker script for init |
| `Makefile` | Modified -- init build target, `-initrd` flag |
| `docs/ROADMAP.md` | Modified -- defer interrupt refactor, update Phase 7 scope |
| `tests/core/test_gdt.cpp` | New -- TSS and user segment tests |
| `tests/core/test_process.cpp` | New -- ProcessManager tests |
| `tests/core/test_elf.cpp` | New -- ELF loader tests |
| `tests/core/test_scheduler.cpp` | New -- scheduler unit tests |
