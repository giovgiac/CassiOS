SERVICE = vga
SOURCES = main.cpp terminal.cpp

ROOT     := $(dir $(lastword $(MAKEFILE_LIST)))../../..
CXXFLAGS = -m32 -mno-sse -mno-sse2 -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-leading-underscore -fno-stack-protector -I$(ROOT)/common/include/ -I$(ROOT)/userspace/libs/libcassio/include/ -Iinclude/ $(foreach dir,$(wildcard $(ROOT)/libs/*/include),-I$(dir)) -MMD -MP
LDFLAGS  = -melf_i386

OBJDIR   = $(ROOT)/obj/userspace/$(SERVICE)
BINDIR   = $(ROOT)/bin
OUTPUT   = $(BINDIR)/$(SERVICE).elf
LIBCOMMON   = $(ROOT)/lib/libcommon.a
LIBSTD_MEM  = $(ROOT)/lib/libstd_mem.a
LIBSTD_STR  = $(ROOT)/lib/libstd_str.a
LIBSTD_ALLOC = $(ROOT)/lib/libstd_alloc.a
LIBSTD_HEAP = $(ROOT)/lib/libstd_heap.a
LIBSTD_OS = $(ROOT)/lib/libstd_os.a
LIBSTD_IPC = $(ROOT)/lib/libstd_ipc.a
LIBSTD_NS = $(ROOT)/lib/libstd_ns.a

OBJECTS  = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

$(OUTPUT): $(OBJECTS) linker.ld $(LIBCOMMON) $(LIBSTD_MEM) $(LIBSTD_STR) $(LIBSTD_ALLOC) $(LIBSTD_HEAP) $(LIBSTD_OS) $(LIBSTD_IPC) $(LIBSTD_NS)
	@mkdir -p $(BINDIR)
	ld $(LDFLAGS) -T linker.ld -o $@ $(OBJECTS) $(LIBSTD_NS) $(LIBSTD_HEAP) $(LIBSTD_OS) $(LIBSTD_IPC) $(LIBSTD_ALLOC) $(LIBSTD_STR) $(LIBSTD_MEM) $(LIBCOMMON)

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(OUTPUT)

-include $(OBJECTS:.o=.d)

.PHONY: clean
