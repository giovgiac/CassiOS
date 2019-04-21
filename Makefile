# Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
# Use of this source code is governed by a MIT-style
# license that can be found in the LICENSE file.

ASMFLAGS = --32
CXXFLAGS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
LDFLAGS = -melf_i386

objects = bin/loader.o bin/kernel.o

%.o: src/*/%.cpp
	g++ $(CXXFLAGS) -o bin/$@ -c $< -I./include/

%.o: src/*/%.s
	as $(ASMFLAGS) -o bin/$@ $<

cassio.bin: src/kernel/linker.ld $(objects)
	ld $(LDFLAGS) -T $< -o bin/$@ $(objects)

