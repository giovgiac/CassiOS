# Common build rules for userspace services.
# Each service Makefile sets SERVICE and SOURCES, then includes this file.

ROOT     = $(dir $(lastword $(MAKEFILE_LIST)))..
CXXFLAGS = -m32 -mno-sse -mno-sse2 -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-leading-underscore -fno-stack-protector -I$(ROOT)/common/include/ -I$(ROOT)/userspace/include/ $(foreach dir,$(wildcard $(ROOT)/libs/*/include),-I$(dir))
LDFLAGS  = -melf_i386

OBJDIR   = $(ROOT)/obj/userspace/$(SERVICE)
BINDIR   = $(ROOT)/bin
OUTPUT   = $(BINDIR)/$(SERVICE).elf
LIBCOMMON   = $(ROOT)/lib/libcommon.a
LIBSTD_MEM  = $(ROOT)/lib/libstd_mem.a
LIBSTD_STR  = $(ROOT)/lib/libstd_str.a
LIBSTD_ALLOC = $(ROOT)/lib/libstd_alloc.a
LIBSTD_HEAP = $(ROOT)/lib/libstd_heap.a

OBJECTS  = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

$(OUTPUT): $(OBJECTS) linker.ld $(LIBCOMMON) $(LIBSTD_MEM) $(LIBSTD_STR) $(LIBSTD_ALLOC) $(LIBSTD_HEAP)
	@mkdir -p $(BINDIR)
	ld $(LDFLAGS) -T linker.ld -o $@ $(OBJECTS) $(LIBSTD_HEAP) $(LIBSTD_ALLOC) $(LIBSTD_STR) $(LIBSTD_MEM) $(LIBCOMMON)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(OUTPUT)

.PHONY: clean
