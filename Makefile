# Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

ASMFLAGS = --32
CXXFLAGS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
LDFLAGS = -melf_i386

objects = loader.o gdt.o iostream.o driver.o port.o stub.o interrupt.o keyboard.o mouse.o kernel.o

%.o: src/*/%.cpp
	mkdir -p obj
	g++ $(CXXFLAGS) -o obj/$@ -c $< -Iinclude/

%.o: src/*/%.s
	mkdir -p obj
	as $(ASMFLAGS) -o obj/$@ $<

cassio.bin: src/linker.ld $(objects)
	mkdir -p bin
	ld $(LDFLAGS) -T $< -o bin/$@ $(addprefix obj/, $(objects))

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
	rm -rf $(addprefix obj/, $(objects)) bin/cassio.bin bin/cassio.iso
