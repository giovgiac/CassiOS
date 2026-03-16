# Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

ASMFLAGS = --32
CXXFLAGS = -m32 -mno-sse -mno-sse2 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -fno-stack-protector
COMMON_CXXFLAGS = -m32 -mno-sse -mno-sse2 -ffreestanding -nostdlib -fno-rtti -fno-exceptions -fno-leading-underscore -fno-stack-protector
LDFLAGS = -melf_i386

KERNEL = bin/cassio.bin
TEST_KERNEL = bin/cassio-test.bin
USERTEST = bin/usertest.elf
INIT = bin/init.elf
ISO = bin/cassio.iso
DISK = bin/disk.img
LIBCOMMON = lib/libcommon.a


# Discover all source files automatically.
cpp_sources = $(shell find kernel/src/ -name '*.cpp')
asm_sources = $(shell find kernel/src/ -name '*.s')
objects = $(patsubst kernel/src/%.cpp, obj/%.o, $(cpp_sources)) $(patsubst kernel/src/%.s, obj/%.o, $(asm_sources))

# Common library sources.
common_sources = $(shell find common/src/ -name '*.cpp')
common_objects = $(patsubst common/src/%.cpp, obj/common/%.o, $(common_sources))

# Shared objects for the test kernel (everything except kernel.o).
shared_objects = $(filter-out obj/core/kernel.o, $(objects))

# Test objects are discovered from kernel/tests/**/test_*.cpp.
test_sources = $(shell find kernel/tests/ -name 'test_*.cpp')
test_objects = $(patsubst kernel/tests/%.cpp, obj/tests/%.o, $(test_sources))

# Compile C++ source files.
obj/%.o: kernel/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $< -Ikernel/include/ -Icommon/include/

# Compile assembly source files.
obj/%.o: kernel/src/%.s
	@mkdir -p $(dir $@)
	as $(ASMFLAGS) -o $@ $<

# Compile common library source files.
obj/common/%.o: common/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(COMMON_CXXFLAGS) -o $@ -c $< -Icommon/include/

$(LIBCOMMON): $(common_objects)
	@mkdir -p lib
	ar rcs $@ $(common_objects)

kernel: kernel/src/linker.ld $(objects) $(LIBCOMMON)
	@mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(KERNEL) $(objects) $(LIBCOMMON)

$(INIT):
	$(MAKE) -C userspace/init

$(USERTEST): $(LIBCOMMON)
	$(MAKE) -C userspace/test

# Compile test files from the kernel/tests/ directory.
obj/tests/%.o: kernel/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $< -Ikernel/include/ -Icommon/include/ -Ikernel/tests/

$(TEST_KERNEL): kernel/src/linker.ld $(shared_objects) $(test_objects) $(LIBCOMMON)
	@mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(TEST_KERNEL) $(shared_objects) $(test_objects) $(LIBCOMMON)

$(DISK):
	@mkdir -p bin
	qemu-img create -f raw $(DISK) 1M

test: test-kernel test-userspace

test-kernel: $(TEST_KERNEL)
	@qemu-img create -f raw /tmp/cassio-test-disk.img 1M 2>/dev/null; \
	qemu-system-i386 -machine pc -kernel $(TEST_KERNEL) \
	    -display none -serial file:/tmp/cassio-test-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -drive file=/tmp/cassio-test-disk.img,format=raw,if=ide \
	    -no-reboot -net none; \
	EXIT_CODE=$$?; \
	rm -f /tmp/cassio-test-disk.img; \
	cat /tmp/cassio-test-results.txt; \
	[ $$EXIT_CODE -eq 1 ]

test-userspace: kernel $(USERTEST)
	@qemu-system-i386 -machine pc -kernel $(KERNEL) \
	    -initrd $(USERTEST) \
	    -display none -serial file:/tmp/cassio-usertest-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -no-reboot -net none; \
	EXIT_CODE=$$?; \
	cat /tmp/cassio-usertest-results.txt; \
	[ $$EXIT_CODE -eq 1 ]

iso: kernel
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp $(KERNEL) iso/boot/
	echo 'set default=0' > iso/boot/grub/grub.cfg
	echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "CassiOS" {' >> iso/boot/grub/grub.cfg
	echo '	multiboot /boot/cassio.bin' >> iso/boot/grub/grub.cfg
	echo '	boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$(ISO) iso
	rm -rf iso

run: kernel $(INIT) $(DISK)
	qemu-system-i386 -machine pc -kernel $(KERNEL) \
	    -initrd $(INIT) \
	    -drive file=$(DISK),format=raw,if=ide

.PHONY: kernel iso clean run test test-kernel test-userspace
clean:
	rm -rf obj/ bin/ lib/
