SERVICE = usertest

ROOT     = $(dir $(lastword $(MAKEFILE_LIST)))../..
NS_DIR   = $(ROOT)/userspace/ns
KBD_DIR  = $(ROOT)/userspace/kbd
CXXFLAGS = -m32 -mno-sse -mno-sse2 -ffreestanding -nostdlib -fno-use-cxa-atexit -fno-exceptions -fno-rtti -fno-leading-underscore -fno-stack-protector -I$(ROOT)/common/include/ -I$(ROOT)/userspace/include/ -I$(NS_DIR)/include/ -I$(KBD_DIR)/include/
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

OBJECTS = $(RUNNER_OBJECTS) $(NS_TEST_OBJECTS) $(NS_IMPL_OBJECTS) $(KBD_TEST_OBJECTS) $(KBD_IMPL_OBJECTS)

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

clean:
	rm -rf $(OBJDIR) $(OUTPUT)

.PHONY: clean
