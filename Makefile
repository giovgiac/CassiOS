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
NAMESERVER = bin/ns.elf
KBD = bin/kbd.elf
VGA = bin/vga.elf
VFS = bin/vfs.elf
MOUSE = bin/mouse.elf
ATA = bin/ata.elf
USERSHELL = bin/shell.elf
ISO = bin/cassio.iso
DISK = bin/disk.img
LIBCOMMON = lib/libcommon.a
LIBCASSIO = lib/libcassio.a


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

# Userspace shared library sources.
cassio_lib_sources = $(shell find userspace/libs/libcassio/src/ -name '*.cpp' 2>/dev/null)
cassio_lib_objects = $(patsubst userspace/libs/libcassio/src/%.cpp, obj/userspace/libs/libcassio/%.o, $(cassio_lib_sources))

# Userspace test: runner + all service tests + all service impls (excluding main.cpp and shared lib).
usertest_sources = userspace/test.cpp \
    $(shell find userspace/ -path '*/tests/test_*.cpp' 2>/dev/null) \
    $(shell find userspace/ -path '*/src/*.cpp' -not -name 'main.cpp' -not -path 'userspace/libs/*' 2>/dev/null)
usertest_objects = $(patsubst userspace/%.cpp, obj/userspace/usertest/%.o, $(usertest_sources))
USERTEST_CXXFLAGS = $(COMMON_CXXFLAGS) -fno-use-cxa-atexit \
    -Icommon/include/ -Iuserspace/libs/libcassio/include/ \
    $(foreach dir,$(shell find userspace/ -type d -name include),-I$(dir))

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

# Compile libcassio source files.
obj/userspace/libs/libcassio/%.o: userspace/libs/libcassio/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(COMMON_CXXFLAGS) -fno-use-cxa-atexit -o $@ -c $< -Icommon/include/ -Iuserspace/libs/libcassio/include/

$(LIBCASSIO): $(cassio_lib_objects)
	@mkdir -p lib
	ar rcs $@ $(cassio_lib_objects)

kernel: kernel/src/linker.ld $(objects) $(LIBCOMMON)
	@mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(KERNEL) $(objects) $(LIBCOMMON)

$(NAMESERVER): $(LIBCOMMON) $(LIBCASSIO)
	$(MAKE) -C userspace/ns

$(KBD): $(LIBCOMMON)
	$(MAKE) -C userspace/drivers/kbd

$(VGA): $(LIBCOMMON)
	$(MAKE) -C userspace/drivers/vga

$(VFS): $(LIBCOMMON)
	$(MAKE) -C userspace/vfs

$(MOUSE): $(LIBCOMMON)
	$(MAKE) -C userspace/drivers/mouse

$(ATA): $(LIBCOMMON)
	$(MAKE) -C userspace/drivers/ata

$(USERSHELL): $(LIBCOMMON)
	$(MAKE) -C userspace/shell

# Compile test files from the kernel/tests/ directory.
obj/tests/%.o: kernel/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $< -Ikernel/include/ -Icommon/include/ -Ikernel/tests/

$(TEST_KERNEL): kernel/src/linker.ld $(shared_objects) $(test_objects) $(LIBCOMMON)
	@mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(TEST_KERNEL) $(shared_objects) $(test_objects) $(LIBCOMMON)

# Compile userspace test files (runner + service tests + service impls).
obj/userspace/usertest/%.o: userspace/%.cpp
	@mkdir -p $(dir $@)
	g++ $(USERTEST_CXXFLAGS) -o $@ -c $<

$(USERTEST): userspace/test.ld $(usertest_objects) $(LIBCOMMON) $(LIBCASSIO)
	@mkdir -p bin
	ld $(LDFLAGS) -T $< -o $@ $(usertest_objects) $(LIBCASSIO) $(LIBCOMMON)

$(DISK):
	@mkdir -p bin
	qemu-img create -f raw $(DISK) 1M

test: test-kernel test-userspace

test-kernel:
	@$(MAKE) --no-print-directory $(TEST_KERNEL) > /tmp/cassio-build.log 2>&1 \
	    || (cat /tmp/cassio-build.log; exit 1); \
	qemu-img create -f raw /tmp/cassio-test-disk.img 1M 2>/dev/null; \
	qemu-system-i386 -machine pc -kernel $(TEST_KERNEL) \
	    -display none -serial file:/tmp/cassio-test-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -drive file=/tmp/cassio-test-disk.img,format=raw,if=ide \
	    -no-reboot; \
	EXIT_CODE=$$?; \
	rm -f /tmp/cassio-test-disk.img; \
	cat /tmp/cassio-test-results.txt; \
	[ $$EXIT_CODE -eq 1 ]

test-userspace:
	@$(MAKE) --no-print-directory kernel $(NAMESERVER) $(KBD) $(VGA) $(VFS) $(MOUSE) $(ATA) $(USERTEST) \
	    > /tmp/cassio-build.log 2>&1 || (cat /tmp/cassio-build.log; exit 1); \
	qemu-img create -f raw /tmp/cassio-usertest-disk.img 1M 2>/dev/null; \
	qemu-system-i386 -machine pc -kernel $(KERNEL) \
	    -initrd "$(NAMESERVER),$(KBD),$(VGA),$(VFS),$(MOUSE),$(ATA),$(USERTEST)" \
	    -display none -serial file:/tmp/cassio-usertest-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -drive file=/tmp/cassio-usertest-disk.img,format=raw,if=ide \
	    -no-reboot; \
	EXIT_CODE=$$?; \
	rm -f /tmp/cassio-usertest-disk.img; \
	cat /tmp/cassio-usertest-results.txt; \
	[ $$EXIT_CODE -eq 1 ]

iso: kernel $(NAMESERVER) $(KBD) $(VGA) $(VFS) $(MOUSE) $(ATA) $(USERSHELL)
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp $(KERNEL) iso/boot/
	cp $(NAMESERVER) $(KBD) $(VGA) $(VFS) $(MOUSE) $(ATA) $(USERSHELL) iso/boot/
	echo 'set default=0' > iso/boot/grub/grub.cfg
	echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "CassiOS" {' >> iso/boot/grub/grub.cfg
	echo '	multiboot /boot/cassio.bin' >> iso/boot/grub/grub.cfg
	echo '	module /boot/ns.elf' >> iso/boot/grub/grub.cfg
	echo '	module /boot/kbd.elf' >> iso/boot/grub/grub.cfg
	echo '	module /boot/vga.elf' >> iso/boot/grub/grub.cfg
	echo '	module /boot/vfs.elf' >> iso/boot/grub/grub.cfg
	echo '	module /boot/mouse.elf' >> iso/boot/grub/grub.cfg
	echo '	module /boot/ata.elf' >> iso/boot/grub/grub.cfg
	echo '	module /boot/shell.elf' >> iso/boot/grub/grub.cfg
	echo '	boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$(ISO) iso
	rm -rf iso

run: kernel $(NAMESERVER) $(KBD) $(VGA) $(VFS) $(MOUSE) $(ATA) $(USERSHELL) $(DISK)
	qemu-system-i386 -machine pc -kernel $(KERNEL) \
	    -initrd "$(NAMESERVER),$(KBD),$(VGA),$(VFS),$(MOUSE),$(ATA),$(USERSHELL)" \
	    -drive file=$(DISK),format=raw,if=ide

.PHONY: kernel iso clean run test test-kernel test-userspace
clean:
	rm -rf obj/ bin/ lib/
