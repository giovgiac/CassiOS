SERVICE = usertest

ROOT     = $(dir $(lastword $(MAKEFILE_LIST)))../..
NS_DIR   = $(ROOT)/userspace/ns
KBD_DIR  = $(ROOT)/userspace/kbd
VGA_DIR  = $(ROOT)/userspace/vga
VFS_DIR  = $(ROOT)/userspace/vfs
MOUSE_DIR = $(ROOT)/userspace/mouse
ATA_DIR  = $(ROOT)/userspace/ata
SHELL_DIR = $(ROOT)/userspace/shell
CXXFLAGS = -m32 -mno-sse -mno-sse2 -ffreestanding -nostdlib -fno-use-cxa-atexit -fno-exceptions -fno-rtti -fno-leading-underscore -fno-stack-protector -I$(ROOT)/common/include/ -I$(ROOT)/userspace/include/ -I$(NS_DIR)/include/ -I$(KBD_DIR)/include/ -I$(VGA_DIR)/include/ -I$(VFS_DIR)/include/ -I$(MOUSE_DIR)/include/ -I$(ATA_DIR)/include/ -I$(SHELL_DIR)/include/
LDFLAGS  = -melf_i386

OBJDIR   = $(ROOT)/obj/userspace/$(SERVICE)
BINDIR   = $(ROOT)/bin
OUTPUT   = $(BINDIR)/$(SERVICE).elf
LIBCOMMON = $(ROOT)/lib/libcommon.a

# Test runner.
RUNNER_OBJECTS = $(OBJDIR)/main.o

# Nameserver tests and implementation (compiled from ns/).
NS_TEST_SOURCES = $(wildcard $(NS_DIR)/tests/test_*.cpp)
NS_IMPL_SOURCES = $(NS_DIR)/src/table.cpp
NS_TEST_OBJECTS = $(patsubst $(NS_DIR)/tests/%.cpp, $(OBJDIR)/ns/tests/%.o, $(NS_TEST_SOURCES))
NS_IMPL_OBJECTS = $(patsubst $(NS_DIR)/src/%.cpp, $(OBJDIR)/ns/src/%.o, $(NS_IMPL_SOURCES))

# Keyboard tests and implementation (compiled from kbd/).
KBD_TEST_SOURCES = $(wildcard $(KBD_DIR)/tests/test_*.cpp)
KBD_IMPL_SOURCES = $(KBD_DIR)/src/keyboard.cpp
KBD_TEST_OBJECTS = $(patsubst $(KBD_DIR)/tests/%.cpp, $(OBJDIR)/kbd/tests/%.o, $(KBD_TEST_SOURCES))
KBD_IMPL_OBJECTS = $(patsubst $(KBD_DIR)/src/%.cpp, $(OBJDIR)/kbd/src/%.o, $(KBD_IMPL_SOURCES))

# VGA tests and implementation (compiled from vga/).
VGA_TEST_SOURCES = $(wildcard $(VGA_DIR)/tests/test_*.cpp)
VGA_IMPL_SOURCES = $(VGA_DIR)/src/terminal.cpp
VGA_TEST_OBJECTS = $(patsubst $(VGA_DIR)/tests/%.cpp, $(OBJDIR)/vga/tests/%.o, $(VGA_TEST_SOURCES))
VGA_IMPL_OBJECTS = $(patsubst $(VGA_DIR)/src/%.cpp, $(OBJDIR)/vga/src/%.o, $(VGA_IMPL_SOURCES))

# VFS tests and implementation (compiled from vfs/).
VFS_TEST_SOURCES = $(wildcard $(VFS_DIR)/tests/test_*.cpp)
VFS_IMPL_SOURCES = $(VFS_DIR)/src/filesystem.cpp
VFS_TEST_OBJECTS = $(patsubst $(VFS_DIR)/tests/%.cpp, $(OBJDIR)/vfs/tests/%.o, $(VFS_TEST_SOURCES))
VFS_IMPL_OBJECTS = $(patsubst $(VFS_DIR)/src/%.cpp, $(OBJDIR)/vfs/src/%.o, $(VFS_IMPL_SOURCES))

# Mouse tests and implementation (compiled from mouse/).
MOUSE_TEST_SOURCES = $(wildcard $(MOUSE_DIR)/tests/test_*.cpp)
MOUSE_IMPL_SOURCES = $(MOUSE_DIR)/src/mouse.cpp
MOUSE_TEST_OBJECTS = $(patsubst $(MOUSE_DIR)/tests/%.cpp, $(OBJDIR)/mouse/tests/%.o, $(MOUSE_TEST_SOURCES))
MOUSE_IMPL_OBJECTS = $(patsubst $(MOUSE_DIR)/src/%.cpp, $(OBJDIR)/mouse/src/%.o, $(MOUSE_IMPL_SOURCES))

# ATA tests (no impl sources needed -- constants only, header-only).
ATA_TEST_SOURCES = $(wildcard $(ATA_DIR)/tests/test_*.cpp)
ATA_TEST_OBJECTS = $(patsubst $(ATA_DIR)/tests/%.cpp, $(OBJDIR)/ata/tests/%.o, $(ATA_TEST_SOURCES))

# Shell tests and implementation (compiled from shell/).
SHELL_TEST_SOURCES = $(wildcard $(SHELL_DIR)/tests/test_*.cpp)
SHELL_IMPL_SOURCES = $(SHELL_DIR)/src/shell.cpp
SHELL_TEST_OBJECTS = $(patsubst $(SHELL_DIR)/tests/%.cpp, $(OBJDIR)/shell/tests/%.o, $(SHELL_TEST_SOURCES))
SHELL_IMPL_OBJECTS = $(patsubst $(SHELL_DIR)/src/%.cpp, $(OBJDIR)/shell/src/%.o, $(SHELL_IMPL_SOURCES))

OBJECTS = $(RUNNER_OBJECTS) $(NS_TEST_OBJECTS) $(NS_IMPL_OBJECTS) $(KBD_TEST_OBJECTS) $(KBD_IMPL_OBJECTS) $(VGA_TEST_OBJECTS) $(VGA_IMPL_OBJECTS) $(VFS_TEST_OBJECTS) $(VFS_IMPL_OBJECTS) $(MOUSE_TEST_OBJECTS) $(MOUSE_IMPL_OBJECTS) $(ATA_TEST_OBJECTS) $(SHELL_TEST_OBJECTS) $(SHELL_IMPL_OBJECTS)

$(OUTPUT): $(OBJECTS) linker.ld $(LIBCOMMON)
	@mkdir -p $(BINDIR)
	ld $(LDFLAGS) -T linker.ld -o $@ $(OBJECTS) $(LIBCOMMON)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/ns/tests/%.o: $(NS_DIR)/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/ns/src/%.o: $(NS_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/kbd/tests/%.o: $(KBD_DIR)/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/kbd/src/%.o: $(KBD_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/vga/tests/%.o: $(VGA_DIR)/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/vga/src/%.o: $(VGA_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/vfs/tests/%.o: $(VFS_DIR)/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/vfs/src/%.o: $(VFS_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/mouse/tests/%.o: $(MOUSE_DIR)/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/mouse/src/%.o: $(MOUSE_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/ata/tests/%.o: $(ATA_DIR)/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/shell/tests/%.o: $(SHELL_DIR)/tests/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/shell/src/%.o: $(SHELL_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(OUTPUT)

.PHONY: clean
