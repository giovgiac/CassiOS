# std::gfx library - stateless 2D graphics primitives

ROOT     = ../..
CXXFLAGS = -m32 -mno-sse -mno-sse2 -ffreestanding -nostdlib -fno-rtti -fno-exceptions -fno-leading-underscore -fno-stack-protector -MMD -MP $(foreach dir,$(wildcard $(ROOT)/libs/*/include),-I$(dir))

OBJDIR   = $(ROOT)/obj/std/gfx
LIBDIR   = $(ROOT)/lib
OUTPUT   = $(LIBDIR)/libstd_gfx.a

SOURCES  = gfx.cpp font.cpp
OBJECTS  = $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

$(OUTPUT): $(OBJECTS)
	@mkdir -p $(LIBDIR)
	ar rcs $@ $(OBJECTS)

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(OUTPUT)

-include $(OBJECTS:.o=.d)

.PHONY: clean
