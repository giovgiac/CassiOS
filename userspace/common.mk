# Common build rules for userspace services.
# Each service Makefile sets SERVICE and SOURCES, then includes this file.

ROOT     = $(dir $(lastword $(MAKEFILE_LIST)))..
CXXFLAGS = -m32 -ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-leading-underscore -fno-stack-protector
LDFLAGS  = -melf_i386

OBJDIR   = $(ROOT)/obj/userspace/$(SERVICE)
BINDIR   = $(ROOT)/bin
OUTPUT   = $(BINDIR)/$(SERVICE).elf

OBJECTS  = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

$(OUTPUT): $(OBJECTS) linker.ld
	@mkdir -p $(BINDIR)
	ld $(LDFLAGS) -T linker.ld -o $@ $(OBJECTS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(OUTPUT)

.PHONY: clean
