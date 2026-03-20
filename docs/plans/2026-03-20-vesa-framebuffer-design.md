# Phase 10: VESA Framebuffer and Graphics Terminal

Switch from VGA text mode to a VESA/VBE linear framebuffer (1024x768x32bpp) and replace the monolithic VGA terminal driver with two services: a pixel-level display driver and a text-rendering terminal service.

## 1. Kernel Changes

### 1a. Multiboot Video Mode Request

Update `kernel/src/core/loader.s` to set flag bit 2 (video mode) in the multiboot header:

```
FLAGS = (1 << 0 | 1 << 1 | 1 << 2)   ; memory info + boot device + video mode
```

Add the video mode fields required when bit 2 is set:

- `mode_type = 0` (linear framebuffer)
- `width = 1024`
- `height = 768`
- `depth = 32`

GRUB reads these and sets up the closest available VESA mode. The actual resolution is reported back in the multiboot info struct.

### 1b. Parse Framebuffer Info from Multiboot

Extend `MultibootInfo` (`kernel/include/memory/multiboot.hpp`) to parse the framebuffer fields: `framebuffer_addr` (u64), `framebuffer_pitch` (u32), `framebuffer_width` (u32), `framebuffer_height` (u32), `framebuffer_bpp` (u8). The kernel stores these at boot for the new syscall.

### 1c. FramebufferInfo Syscall

New syscall that returns the framebuffer physical address, width, height, pitch, and bpp. The display service calls this on startup to learn where and how to map the framebuffer.

### 1d. Relax MapDevice Restriction

The current `MapDevice` syscall rejects physical addresses above 1 MB. The VESA framebuffer lives at a high physical address (typically 0xFD000000+). Relax the check to allow any physical address that doesn't overlap kernel memory. Increase the page count limit from 256 -- a 1024x768x32bpp framebuffer is ~3 MiB = 768 pages.

## 2. Graphics Library (`std::gfx`)

New stateless library under `libs/gfx/` in the `std::gfx` namespace. Pure pixel manipulation on caller-provided buffers -- no ownership, no display knowledge, no IPC.

### Data Types

- `Color` -- `u32` (0x00RRGGBB, alpha byte ignored)
- `PixelBuffer` -- struct: `u32* data`, `u32 width`, `u32 height`, `u32 pitch` (bytes per row)

### Functions

- `drawPixel(buf, x, y, color)` -- set one pixel
- `fillRect(buf, x, y, w, h, color)` -- fill rectangle with solid color
- `drawRect(buf, x, y, w, h, color)` -- draw 1px rectangle outline
- `drawChar(buf, x, y, ch, fg, bg)` -- render one 8x16 glyph from the embedded font
- `drawText(buf, x, y, text, len, fg, bg)` -- render a string (calls drawChar in a loop)
- `scroll(buf, pixels, color)` -- shift buffer up by N pixel rows, fill gap with color
- `blit(dst, dx, dy, src, sx, sy, w, h)` -- copy rectangular region between buffers

### Font Data

The classic 8x16 VGA/CP437 bitmap font stored as `const u8 font[256][16]` (4 KiB). Each glyph is 16 bytes, one byte per row, MSB = leftmost pixel. `drawChar` iterates 16 rows x 8 columns, writing fg or bg color.

### Testing

Unit tests allocate a buffer with `new u32[w*h]`, call gfx functions, assert pixel values. No hardware or IPC needed.

## 3. Display Service (`userspace/drivers/display/`)

Replaces the VGA text-mode driver. Owns the framebuffer and back buffer, exposes pixel-level IPC commands. Registers as `"display"` with the nameserver.

### Startup

1. Register as `"display"` with nameserver
2. Call `FramebufferInfo` syscall -- get physical address, width, height, pitch, bpp
3. Call `MapDevice` to map the framebuffer into its address space
4. Allocate a back buffer of equal size via `sbrk`
5. Clear the back buffer and flush to framebuffer

### Internal Structure

- `Display` class -- owns the framebuffer pointer, back buffer (as a `PixelBuffer`), and screen dimensions. Methods correspond 1:1 to IPC commands. Uses `std::gfx` internally for all pixel manipulation on the back buffer. `flush()` copies back buffer to framebuffer via `std::mem::copy`.
- `main.cpp` -- entry point, IPC receive loop, dispatches to Display methods.

### IPC Commands

| Message Type | Args | Data Buffer | Reply |
|---|---|---|---|
| `DisplayFillRect` | x, y, w, h, color | -- | -- |
| `DisplayDrawRect` | x, y, w, h, color | -- | -- |
| `DisplayBlit` | x, y, w, h | pixel data (w*h*4 bytes) | -- |
| `DisplayScroll` | pixels, color | -- | -- |
| `DisplayFlush` | -- | -- | -- |
| `DisplayGetInfo` | -- | -- | width, height, pitch, bpp |

All draw commands operate on the back buffer. Nothing reaches the screen until `DisplayFlush`. `DisplayGetInfo` is blocking (needs reply); the rest are fire-and-forget via notify except `DisplayBlit` which uses send/reply for the data buffer.

### Directory Structure

```
userspace/drivers/display/
  include/display.hpp
  src/display.cpp
  src/main.cpp
  tests/test_display.cpp
  tests/test_ipc.cpp
```

## 4. Terminal Service (`userspace/core/terminal/`)

Sits between shell/apps and the display service. Maintains a character grid, renders text, translates to display IPC commands. Registers as `"terminal"` with the nameserver.

### Startup

1. Register as `"terminal"` with nameserver
2. Connect to display service via `std::display::Display` client
3. Query `DisplayGetInfo` to learn screen dimensions
4. Compute grid size: `cols = width / 8`, `rows = height / 16` (128x48 at 1024x768)
5. Clear screen, print welcome message

### Internal Structure

- `Terminal` class -- cursor position (x, y), color (fg, bg), grid dimensions (cols, rows). `putchar` handles control characters (newline, backspace, delete, tab) exactly as the current `VgaTerminal`, but instead of writing to a VGA text buffer it sends draw commands to the display service.
- `main.cpp` -- entry point, IPC receive loop.

### Glyph Rendering

For each printable character, the terminal:

1. Renders the glyph into a small 8x16 local buffer using `std::gfx::drawChar`
2. Calls `display.blit(x*8, y*16, 8, 16, glyphPixels)` to send it to the display

For backspace/delete, calls `display.fillRect` to clear the cell. For scroll, calls `display.scroll`. For clear, calls `display.fillRect` over the full screen.

### Flushing Strategy

The terminal calls `display.flush()` after processing each IPC message. A `TerminalWrite` of N characters does N draw operations but only one flush at the end. A `TerminalPutchar` draws one character and flushes.

### IPC Protocol

Same message types as the current VGA service, renamed: `TerminalPutchar`, `TerminalWrite`, `TerminalClear`, `TerminalSetCursor`, `TerminalGetCursor`. The shell and apps use the same API -- just resolve `"terminal"` instead of `"vga"`.

### Directory Structure

```
userspace/core/terminal/
  include/terminal.hpp
  src/terminal.cpp
  src/main.cpp
  tests/test_terminal.cpp
  tests/test_ipc.cpp
```

## 5. Client Libraries and Migration

### New Client Libraries

- `std::display::Display` (`libs/display/`) -- client for the display service. Constructor resolves `"display"` from nameserver. Methods: `fillRect`, `drawRect`, `blit`, `scroll`, `flush`, `getInfo`.
- `std::terminal::Terminal` (`libs/terminal/`) -- client for the terminal service. Constructor resolves `"terminal"` from nameserver. Same API as current `std::vga::Vga`: `putchar`, `write`, `clear`, `setCursor`, `getCursor`. Drop-in replacement.

### Migration Steps

1. Rename IPC message types: `VgaPutchar` -> `TerminalPutchar`, etc. Add new `Display*` types.
2. Replace `std::vga::Vga` usage in shell and all apps with `std::terminal::Terminal`.
3. Remove the old VGA driver (`userspace/drivers/vga/`) entirely.
4. Update QEMU `-initrd` line: drop `vga.elf`, add `display.elf` and `terminal.elf`.
5. Update `make run`, `make test`, `make iso`, and the headless screenshot recipe.

### Boot Order

Display must start before terminal (terminal connects to display). Terminal must start before shell (shell connects to terminal). The `-initrd` order: `ns.elf, kbd.elf, display.elf, mouse.elf, ata.elf, terminal.elf, shell.elf`.

## Design Decisions

- **Resolution**: 1024x768x32bpp requested via multiboot header. Display service reads actual resolution from multiboot info, works regardless of what GRUB negotiates.
- **Font**: Embedded 8x16 VGA/CP437 bitmap font (4 KiB). Gives 128x48 character grid at 1024x768.
- **Double buffering**: All draws go to a back buffer; `DisplayFlush` copies to the framebuffer. No tearing.
- **Single client at a time**: No compositing. Terminal owns the display by default. Future graphics apps can request exclusive access (terminal redraws on return). Window manager is a future phase.
- **Back buffer allocation**: Via `sbrk` in the display service. No new memory syscalls needed.
- **Framebuffer discovery**: `FramebufferInfo` syscall returns physical address and dimensions from multiboot info.
- **No text-mode fallback**: QEMU and all modern VBE hardware support 1024x768x32bpp. No fallback path to VGA text mode.

## Implementation Notes

Key deviations from the design during implementation:

- **PixelBuffer ring buffer scroll**: The design described PixelBuffer as stateless, but the implementation uses a ring buffer with a `scrollOffset` field for O(1) scrolling without copying pixel data.
- **Framebuffer virtual address**: The display service maps the framebuffer at virtual address `0x10000000`. The design did not specify where in the address space to map it.
- **Terminal character grid**: The terminal maintains a `cells` array (heap-allocated `char` grid) to track character content at each position. Used for cursor rendering (save/restore character under cursor). The design did not mention this data structure.
- **Explicit terminal flush**: The shell calls `terminal.flush()` explicitly to trigger display updates. The design assumed flushing would happen implicitly on blocking sends.
- **DisplayDrawChar command**: A `DisplayDrawChar` IPC command was added so the terminal can ask the display to render a glyph directly, avoiding the overhead of blitting an 8x16 pixel buffer per character. The design only had pixel-level commands.
- **Optimized mem::copy/move/set**: `std::mem::copy`, `std::mem::move`, and `std::mem::set` were optimized with alignment-safe `u32`-width copies to improve framebuffer transfer performance.
- **IPC message types renumbered**: Message types were renumbered contiguously to accommodate the new Display and Terminal types, rather than appending to the existing sparse numbering.
