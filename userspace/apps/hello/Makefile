SERVICE = hello

ROOT     := $(dir $(lastword $(MAKEFILE_LIST)))../../..
TARGET   = i586-unknown-linux-gnu
LDFLAGS  = -melf_i386 --gc-sections --strip-debug -u _start

DISKDIR  = $(ROOT)/disk/bin
OUTPUT   = $(DISKDIR)/$(SERVICE).elf
STATICLIB = $(ROOT)/target/$(TARGET)/release/lib$(SERVICE).a

$(OUTPUT): FORCE linker.ld
	cargo build --release --target $(TARGET) --target-dir $(ROOT)/target -p $(SERVICE)
	@mkdir -p $(DISKDIR)
	ld $(LDFLAGS) -T linker.ld -o $@ $(STATICLIB)

FORCE:

clean:
	rm -f $(OUTPUT)

.PHONY: clean FORCE
