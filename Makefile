# Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

ASMFLAGS = --32
CXXFLAGS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -fno-stack-protector
LDFLAGS = -melf_i386

KERNEL = bin/cassio.bin
TEST_KERNEL = bin/cassio-test.bin
ISO = bin/cassio.iso

objects = loader.o gdt.o shell.o driver.o port.o terminal.o stub.o interrupt.o keyboard.o mouse.o physical.o heap.o operators.o paging.o kernel.o

# Shared objects for the test kernel (everything except kernel.o)
shared_objects = loader.o gdt.o shell.o driver.o port.o serial.o terminal.o stub.o interrupt.o keyboard.o mouse.o physical.o heap.o operators.o paging.o

# Test objects are discovered from tests/test_*.cpp
test_sources = $(wildcard tests/test_*.cpp)
test_objects = $(notdir $(test_sources:.cpp=.o))

%.o: src/*/%.cpp
	mkdir -p obj
	g++ $(CXXFLAGS) -o obj/$@ -c $< -Iinclude/

%.o: src/*/%.s
	mkdir -p obj
	as $(ASMFLAGS) -o obj/$@ $<

kernel: src/linker.ld $(objects)
	mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(KERNEL) $(addprefix obj/, $(objects))

# Compile test files from the tests/ directory
obj/test_%.o: tests/test_%.cpp
	mkdir -p obj
	g++ $(CXXFLAGS) -o $@ -c $< -Iinclude/ -Itests/

$(TEST_KERNEL): src/linker.ld $(shared_objects) $(addprefix obj/, $(test_objects))
	mkdir -p bin
	ld $(LDFLAGS) -T $< -o $(TEST_KERNEL) $(addprefix obj/, $(shared_objects)) $(addprefix obj/, $(test_objects))

test: $(TEST_KERNEL)
	@qemu-system-i386 -machine pc -kernel $(TEST_KERNEL) \
	    -display none -serial file:/tmp/cassio-test-results.txt \
	    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	    -no-reboot -net none; \
	EXIT_CODE=$$?; \
	cat /tmp/cassio-test-results.txt; \
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

run: kernel
	qemu-system-i386 -machine pc -kernel $(KERNEL) -net none

.PHONY: kernel iso clean run test
clean:
	rm -rf obj/ bin/
