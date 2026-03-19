SERVICE = vga
SOURCES = main.cpp terminal.cpp

ROOT     = $(dir $(lastword $(MAKEFILE_LIST)))../../..
CXXFLAGS = -m32 -mno-sse -mno-sse2 -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-leading-underscore -fno-stack-protector -I$(ROOT)/common/include/ -I$(ROOT)/userspace/libs/libcassio/include/ -Iinclude/ $(foreach dir,$(wildcard $(ROOT)/libs/*/include),-I$(dir)) -MMD -MP
LDFLAGS  = -melf_i386

OBJDIR   = $(ROOT)/obj/userspace/$(SERVICE)
BINDIR   = $(ROOT)/bin
OUTPUT   = $(BINDIR)/$(SERVICE).elf
LIBCOMMON   = $(ROOT)/lib/libcommon.a
LIBSTD_MEM  = $(ROOT)/lib/libstd_mem.a

OBJECTS  = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

$(OUTPUT): $(OBJECTS) linker.ld $(LIBCOMMON) $(LIBSTD_MEM)
	@mkdir -p $(BINDIR)
	ld $(LDFLAGS) -T linker.ld -o $@ $(OBJECTS) $(LIBSTD_MEM) $(LIBCOMMON)

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(OUTPUT)

-include $(OBJECTS:.o=.d)

.PHONY: clean
