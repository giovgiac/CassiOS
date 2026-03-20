#include <std/test.hpp>

#include <core/elf.hpp>
#include <memory/paging.hpp>
#include <memory/physical.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::kernel;
using namespace cassio::memory;

// Build a minimal valid ELF32 executable with one PT_LOAD segment.
// The segment loads 4 bytes of code at virtual address 0x00400000.
static void buildMinimalElf(u8* buf, u32& size) {
    // Zero the buffer.
    for (u32 i = 0; i < 256; i++)
        buf[i] = 0;

    // ELF header (52 bytes).
    Elf32Header* h = (Elf32Header*)buf;
    h->e_ident[0] = 0x7F;
    h->e_ident[1] = 'E';
    h->e_ident[2] = 'L';
    h->e_ident[3] = 'F';
    h->e_ident[4] = 1; // ELFCLASS32
    h->e_ident[5] = 1; // little-endian
    h->e_ident[6] = 1; // ELF version
    h->e_type = 2;     // ET_EXEC
    h->e_machine = 3;  // EM_386
    h->e_version = 1;
    h->e_entry = 0x00400000;
    h->e_phoff = sizeof(Elf32Header);
    h->e_phentsize = sizeof(Elf32ProgramHeader);
    h->e_phnum = 1;
    h->e_ehsize = sizeof(Elf32Header);

    // Program header (32 bytes), immediately after ELF header.
    Elf32ProgramHeader* ph = (Elf32ProgramHeader*)(buf + sizeof(Elf32Header));
    ph->p_type = 1;     // PT_LOAD
    ph->p_offset = 128; // data starts at byte 128 in the file
    ph->p_vaddr = 0x00400000;
    ph->p_paddr = 0x00400000;
    ph->p_filesz = 4;
    ph->p_memsz = 4;
    ph->p_flags = 5; // PF_R | PF_X
    ph->p_align = 0x1000;

    // 4 bytes of "code" at offset 128.
    buf[128] = 0xDE;
    buf[129] = 0xAD;
    buf[130] = 0xBE;
    buf[131] = 0xEF;

    size = 132;
}

TEST(elf_load_valid_returns_entry_point) {
    u8 buf[256];
    u32 size;
    buildMinimalElf(buf, size);

    PagingManager& pm = PagingManager::getManager();
    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);

    ElfLoadResult result = ElfLoader::load(pd, buf, size);
    ASSERT(result.success);
    ASSERT_EQ(result.entryPoint, 0x00400000u);
    // Segment at 0x00400000 with 4 bytes -> heapStart = 0x00401000 (page-aligned).
    ASSERT_EQ(result.heapStart, 0x00401000u);

    pm.destroyAddressSpace(pd);
}

TEST(elf_load_invalid_magic_fails) {
    u8 buf[256];
    u32 size;
    buildMinimalElf(buf, size);

    // Corrupt magic byte.
    buf[0] = 0x00;

    PagingManager& pm = PagingManager::getManager();
    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);

    ElfLoadResult result = ElfLoader::load(pd, buf, size);
    ASSERT(!result.success);

    pm.destroyAddressSpace(pd);
}

TEST(elf_load_wrong_machine_fails) {
    u8 buf[256];
    u32 size;
    buildMinimalElf(buf, size);

    // Change machine to ARM (0x28) instead of i386 (0x03).
    Elf32Header* h = (Elf32Header*)buf;
    h->e_machine = 0x28;

    PagingManager& pm = PagingManager::getManager();
    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);

    ElfLoadResult result = ElfLoader::load(pd, buf, size);
    ASSERT(!result.success);

    pm.destroyAddressSpace(pd);
}

TEST(elf_load_p_offset_overflow_rejected) {
    u8 buf[256];
    u32 size;
    buildMinimalElf(buf, size);

    // Set p_offset near 0xFFFFFFFF to trigger u32 wrap.
    Elf32ProgramHeader* ph = (Elf32ProgramHeader*)(buf + sizeof(Elf32Header));
    ph->p_offset = 0xFFFFFFF0;

    PagingManager& pm = PagingManager::getManager();
    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);

    ElfLoadResult result = ElfLoader::load(pd, buf, size);
    ASSERT(!result.success);

    pm.destroyAddressSpace(pd);
}

TEST(elf_load_p_offset_beyond_file_rejected) {
    u8 buf[256];
    u32 size;
    buildMinimalElf(buf, size);

    // Set p_offset past the end of the ELF data.
    Elf32ProgramHeader* ph = (Elf32ProgramHeader*)(buf + sizeof(Elf32Header));
    ph->p_offset = size + 1;

    PagingManager& pm = PagingManager::getManager();
    u32 pd = pm.createAddressSpace();
    ASSERT(pd != 0);

    ElfLoadResult result = ElfLoader::load(pd, buf, size);
    ASSERT(!result.success);

    pm.destroyAddressSpace(pd);
}
