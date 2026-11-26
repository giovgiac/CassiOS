SERVICE = mouse

ROOT     := $(dir $(lastword $(MAKEFILE_LIST)))../../..
TARGET   = i586-unknown-linux-gnu
LDFLAGS  = -melf_i386 --gc-sections --strip-debug -u _start

BINDIR   = $(ROOT)/bin
OUTPUT   = $(BINDIR)/$(SERVICE).elf
STATICLIB = $(ROOT)/target/$(TARGET)/release/lib$(SERVICE).a
LIBSTD_MEM = $(ROOT)/lib/libstd_mem.a

$(OUTPUT): FORCE linker.ld $(LIBSTD_MEM)
	cargo build --release --target $(TARGET) --target-dir $(ROOT)/target -p $(SERVICE)
	@mkdir -p $(BINDIR)
	ld $(LDFLAGS) -T linker.ld -o $@ $(STATICLIB) $(LIBSTD_MEM)

FORCE:

clean:
	rm -f $(OUTPUT)

.PHONY: clean FORCE
