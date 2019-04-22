# Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

ASMFLAGS = --32
CXXFLAGS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
LDFLAGS = -melf_i386

objects = loader.o gdt.o port.o stub.o interrupt.o kernel.o

%.o: src/*/%.cpp
	g++ $(CXXFLAGS) -o bin/$@ -c $< -I./include/

%.o: src/*/%.s
	as $(ASMFLAGS) -o bin/$@ $<

cassio.bin: src/linker.ld $(objects)
	ld $(LDFLAGS) -T $< -o bin/$@ $(addprefix bin/, $(objects))

cassio.iso: cassio.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp bin/$< iso/boot/
	echo 'set default=0' > iso/boot/grub/grub.cfg
	echo 'set timeout=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "CassiOS" {' >> iso/boot/grub/grub.cfg
	echo '	multiboot /boot/cassio.bin' >> iso/boot/grub/grub.cfg
	echo '	boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=bin/$@ iso
	rm -rf iso

run: cassio.iso
	VirtualBox --startvm "CassiOS" &

.PHONY: clean
clean:
	rm -rf $(addprefix bin/, $(objects)) bin/cassio.bin bin/cassio.iso
