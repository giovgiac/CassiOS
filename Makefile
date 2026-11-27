# Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

DISK_SIZE_MB ?= 32
# Seconds a QEMU test run may take; a hung run fails instead of blocking.
TEST_TIMEOUT ?= 120

ASMFLAGS = --32
LDFLAGS = -melf_i386 --gc-sections
RUST_TARGET = i586-unknown-linux-gnu

KERNEL = bin/cassio.bin
TEST_KERNEL = bin/cassio-test.bin
USERTEST = bin/usertest.elf
NAMESERVER = bin/ns.elf
KBD = bin/kbd.elf
VFS = bin/vfs.elf
MOUSE = bin/mouse.elf
ATA = bin/ata.elf
DISPLAY = bin/display.elf
TERMINAL = bin/terminal.elf
USERSHELL = bin/shell.elf
HELLO = disk/bin/hello.elf
ISO = bin/cassio.iso
DISK = bin/disk.img
LIBSTD_MEM = lib/libstd_mem.a
KERNEL_LIB = target/$(RUST_TARGET)/release/libkernel.a
# The test kernel's library (feature ktest), in its own target directory.
TEST_KERNEL_LIB = target/ktest/$(RUST_TARGET)/release/libkernel.a


# The kernel is Rust plus three assembly files: the multiboot loader and
# the interrupt and syscall entry stubs.
asm_sources = $(shell find kernel/src/ -name '*.s')
objects = $(patsubst kernel/src/%.s, obj/%.o, $(asm_sources))

# Compile assembly source files.
obj/%.o: kernel/src/%.s
	@mkdir -p $(dir $@)
	as $(ASMFLAGS) -o $@ $<

$(LIBSTD_MEM): FORCE
	$(MAKE) -C libs/mem

$(KERNEL_LIB): FORCE
	cargo build --release --target $(RUST_TARGET) --target-dir target -p kernel

$(TEST_KERNEL_LIB): FORCE
	cargo build --release --target $(RUST_TARGET) --target-dir target/ktest -p kernel --features ktest

FORCE:

kernel: kernel/src/linker.ld $(objects) $(KERNEL_LIB) $(LIBSTD_MEM)
	@mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(KERNEL) $(objects) $(KERNEL_LIB) $(LIBSTD_MEM)

$(NAMESERVER): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/core/ns

$(KBD): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/drivers/kbd

$(VFS): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/core/vfs

$(MOUSE): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/drivers/mouse

$(ATA): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/drivers/ata

$(DISPLAY): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/drivers/display

$(TERMINAL): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/core/terminal

$(USERSHELL): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/core/shell

$(HELLO): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/apps/hello

$(TEST_KERNEL): kernel/src/linker.ld $(objects) $(TEST_KERNEL_LIB) $(LIBSTD_MEM)
	@mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(TEST_KERNEL) $(objects) $(TEST_KERNEL_LIB) $(LIBSTD_MEM)

$(USERTEST): FORCE $(LIBSTD_MEM)
	$(MAKE) -C userspace/tests

disk_files = $(shell find disk/ -type f 2>/dev/null)
$(DISK): $(disk_files) $(HELLO)
	@mkdir -p bin
	dd if=/dev/zero of=$(DISK) bs=1M count=$(DISK_SIZE_MB) 2>/dev/null
	mkfs.fat -F 32 $(DISK) >/dev/null 2>&1
	@if [ -d disk ]; then \
	    for f in $$(find disk/ -type f); do \
	        rel=$${f#disk/}; \
	        dir=$$(dirname "$$rel"); \
	        if [ "$$dir" != "." ]; then \
	            mmd -i $(DISK) "::/$${dir}" 2>/dev/null || true; \
	        fi; \
	        mcopy -i $(DISK) "$$f" "::/$${rel}"; \
	    done; \
	fi

test: test-rust test-kernel test-userspace

# Host unit tests for the Rust crates.
test-rust:
	@cargo test --workspace --tests

test-kernel:
	@$(MAKE) --no-print-directory $(TEST_KERNEL) > /tmp/cassio-build.log 2>&1 \
	    || { cat /tmp/cassio-build.log; exit 1; }; \
	dd if=/dev/zero of=/tmp/cassio-test-disk.img bs=1M count=32 2>/dev/null; \
	mkfs.fat -F 32 /tmp/cassio-test-disk.img >/dev/null 2>&1; \
	timeout $(TEST_TIMEOUT) qemu-system-i386 -machine pc -kernel $(TEST_KERNEL) \
	    -display none -serial file:/tmp/cassio-test-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -drive file=/tmp/cassio-test-disk.img,format=raw,if=ide \
	    -no-reboot 2>/dev/null; \
	EXIT_CODE=$$?; \
	rm -f /tmp/cassio-test-disk.img; \
	cat /tmp/cassio-test-results.txt; \
	[ $$EXIT_CODE -eq 1 ]

test-userspace:
	@$(MAKE) --no-print-directory kernel $(NAMESERVER) $(KBD) $(VFS) $(MOUSE) $(ATA) $(DISPLAY) $(TERMINAL) $(USERTEST) $(HELLO) \
	    > /tmp/cassio-build.log 2>&1 || { cat /tmp/cassio-build.log; exit 1; }; \
	dd if=/dev/zero of=/tmp/cassio-usertest-disk.img bs=1M count=32 2>/dev/null; \
	mkfs.fat -F 32 /tmp/cassio-usertest-disk.img >/dev/null 2>&1; \
	if [ -d disk ]; then \
	    for f in $$(find disk/ -type f); do \
	        rel=$${f#disk/}; \
	        dir=$$(dirname "$$rel"); \
	        if [ "$$dir" != "." ]; then \
	            mmd -i /tmp/cassio-usertest-disk.img "::/$${dir}" 2>/dev/null || true; \
	        fi; \
	        mcopy -i /tmp/cassio-usertest-disk.img "$$f" "::/$${rel}"; \
	    done; \
	fi; \
	timeout $(TEST_TIMEOUT) qemu-system-i386 -machine pc -kernel $(KERNEL) \
	    -initrd "$(NAMESERVER),$(KBD),$(VFS),$(MOUSE),$(ATA),$(DISPLAY),$(TERMINAL),$(USERTEST)" \
	    -display none -serial file:/tmp/cassio-usertest-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -drive file=/tmp/cassio-usertest-disk.img,format=raw,if=ide \
	    -no-reboot 2>/dev/null; \
	EXIT_CODE=$$?; \
	rm -f /tmp/cassio-usertest-disk.img; \
	cat /tmp/cassio-usertest-results.txt; \
	[ $$EXIT_CODE -eq 1 ]

$(ISO): kernel $(NAMESERVER) $(KBD) $(VFS) $(MOUSE) $(ATA) $(DISPLAY) $(TERMINAL) $(USERSHELL)
	@rm -rf iso
	@mkdir -p iso/boot/grub
	@cp $(KERNEL) iso/boot/
	@cp $(NAMESERVER) $(KBD) $(VFS) $(MOUSE) $(ATA) $(DISPLAY) $(TERMINAL) $(USERSHELL) iso/boot/
	@echo 'set default=0' > iso/boot/grub/grub.cfg
	@echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	@echo 'menuentry "CassiOS" {' >> iso/boot/grub/grub.cfg
	@echo '	multiboot /boot/cassio.bin' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/ns.elf' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/kbd.elf' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/vfs.elf' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/mouse.elf' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/ata.elf' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/display.elf' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/terminal.elf' >> iso/boot/grub/grub.cfg
	@echo '	module /boot/shell.elf' >> iso/boot/grub/grub.cfg
	@echo '	boot' >> iso/boot/grub/grub.cfg
	@echo '}' >> iso/boot/grub/grub.cfg
	@echo '' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$(ISO) iso 2>/dev/null
	@rm -rf iso

run: $(ISO) $(DISK)
	qemu-system-i386 -machine pc -cdrom $(ISO) -boot d \
	    -drive file=$(DISK),format=raw,if=ide

.PHONY: kernel clean run test test-rust test-kernel test-userspace
clean:
	rm -rf obj/ bin/ lib/ target/
