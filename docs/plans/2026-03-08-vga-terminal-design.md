# VGA Terminal Design

Replace `std::ostream` with a `Terminal` base class and `VgaTerminal` singleton. The current ostream mixes stream and terminal responsibilities -- it writes directly to VGA memory, manages cursor position, and interprets control characters. Since there is no need for a stream abstraction at this stage, we replace it with a terminal class that owns these responsibilities explicitly.

## Components

### Terminal (base class)

Virtual interface for any text output device. Located in `include/hardware/terminal.hpp`.

```
class Terminal {
public:
    virtual ~Terminal();
    virtual void putchar(char ch) = 0;
    virtual void print(const char* str) = 0;
    virtual void print_hex(u32 value) = 0;
    virtual void clear() = 0;
};
```

Default constructor only. Copy/move not deleted at the base level -- left to concrete subclasses.

### VgaTerminal (singleton)

Inherits from `Terminal`. Singleton bound to the VGA text buffer at 0xB8000. Located in `include/hardware/terminal.hpp` alongside `Terminal`.

Private members:
- `u16* buffer` -- pointer to 0xB8000
- `u8 x, y` -- cursor position

Public interface:
- `static VgaTerminal& getTerminal()` -- singleton accessor
- Overrides: `putchar`, `print`, `print_hex`, `clear`
- Deleted copy/move constructors and assignment operators

#### Control characters handled by putchar

- `'\n'` (0x0A) -- move cursor to start of next line
- `'\b'` (0x08) -- move cursor back one position, erase character
- `0x7F` (DEL) -- erase character at cursor position, no cursor movement
- `'\t'` (0x09) -- advance cursor to next 8-column tab stop

All other characters are written as visible glyphs to VGA memory.

#### Terminal behavior

- Characters written as `(attribute << 8) | char` -- preserving the existing color attribute byte
- Auto-wrap at 80 columns
- Auto-clear and reset to (0, 0) when row 25 is reached
- `print_hex` outputs with `0x` prefix, 8 hex digits (e.g., `0x0000001A`)
- `clear` fills all 2000 entries with space (0x20) while preserving attribute bytes

## Files

### Added
- `include/hardware/terminal.hpp` -- Terminal base class and VgaTerminal
- `src/hardware/terminal.cpp` -- implementation
- `tests/test_terminal.cpp` -- VgaTerminal tests

### Removed
- `include/std/iostream.hpp`
- `src/std/iostream.cpp`
- `tests/test_iostream.cpp`

### Modified
- `include/core/kernel.hpp` -- replace `#include <std/iostream.hpp>` with `#include <hardware/terminal.hpp>`
- `include/hardware/interrupt.hpp` -- replace `#include <std/iostream.hpp>` with `#include <hardware/terminal.hpp>`
- `include/drivers/keyboard.hpp` -- remove unused `#include <std/iostream.hpp>`
- `src/core/kernel.cpp` -- migrate to VgaTerminal API, lowercase boot messages
- `src/hardware/interrupt.cpp` -- migrate to VgaTerminal API
- `Makefile` -- swap iostream.o for terminal.o, swap test_iostream.o for test_terminal.o

## Caller migration

### kernel.cpp

Boot messages use `VgaTerminal::getTerminal()` with `print()` and `clear()`. Messages changed to normal case:
- "Welcome to CassiOS!"
- "Starting up drivers..."
- "Finished starting up drivers."

Keyboard handler calls `putchar()` for character echo and newline.

### interrupt.cpp

Unhandled interrupt warning uses `print()` and `print_hex()`:
```
vga.print("Unhandled Interrupt ");
vga.print_hex(number);
vga.print(" Triggered!\n");
```

## Tests

Rewrite `test_iostream.cpp` as `test_terminal.cpp` with:

- `terminal_putchar` -- write a character, verify it appears in VGA memory
- `terminal_preserves_attribute` -- write a character, verify color attribute byte is unchanged
- `terminal_clear` -- verify clear fills buffer with spaces, preserves attributes
- `terminal_newline` -- verify `'\n'` advances cursor to next line
- `terminal_backspace` -- verify `'\b'` moves cursor back and erases character
