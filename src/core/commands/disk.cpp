/**
 * disk.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "core/commands/disk.hpp"
#include "drivers/ata.hpp"
#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::kernel;
using namespace cassio::filesystem;
using namespace cassio::hardware;

static u32 parse_u32(const char* str) {
    u32 result = 0;
    for (usize i = 0; str[i] >= '0' && str[i] <= '9'; ++i) {
        result = result * 10 + static_cast<u32>(str[i] - '0');
    }
    return result;
}

// --- atainfo ---

static AtaInfoCommand ataInfoInstance;

AtaInfoCommand::AtaInfoCommand() : Command("atainfo", "Show ATA drive info") {}

bool AtaInfoCommand::execute(const char** args, usize argc,
                             FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    AtaPioDriver& ata = AtaPioDriver::getDriver();

    if (!ata.isPresent()) {
        vga.print("No ATA drive detected\n");
        return true;
    }

    vga.print("ATA drive:\n");
    vga.print("  Model:    ");
    vga.print(ata.getModel());
    vga.putchar('\n');
    vga.print("  Sectors:  ");
    vga.print_dec(ata.getSectors());
    vga.putchar('\n');
    vga.print("  Capacity: ");
    u32 kb = ata.getSectors() / 2;
    vga.print_dec(kb);
    vga.print(" KiB\n");

    return true;
}

// --- ataread ---

static AtaReadCommand ataReadInstance;

AtaReadCommand::AtaReadCommand() : Command("ataread", "Read ATA sector(s)") {}

bool AtaReadCommand::execute(const char** args, usize argc,
                             FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    AtaPioDriver& ata = AtaPioDriver::getDriver();

    if (argc < 2) {
        vga.print("Usage: ataread <lba> [count]\n");
        return true;
    }

    if (!ata.isPresent()) {
        vga.print("No ATA drive detected\n");
        return true;
    }

    u32 lba = parse_u32(args[1]);
    u32 count = (argc >= 3) ? parse_u32(args[2]) : 1;

    u8 buffer[ATA_SECTOR_SIZE];

    for (u32 s = 0; s < count; ++s) {
        if (!ata.readSector(lba + s, buffer)) {
            vga.print("Read error at sector ");
            vga.print_dec(lba + s);
            vga.putchar('\n');
            return true;
        }

        vga.print("Sector ");
        vga.print_dec(lba + s);
        vga.print(":\n");

        // Hex dump: 16 bytes per line, 32 lines = 512 bytes.
        for (u32 row = 0; row < 32; ++row) {
            vga.print_hex(row * 16);
            vga.print("  ");
            for (u32 col = 0; col < 16; ++col) {
                u8 b = buffer[row * 16 + col];
                // Print two hex digits.
                const char* hex = "0123456789ABCDEF";
                vga.putchar(hex[(b >> 4) & 0xF]);
                vga.putchar(hex[b & 0xF]);
                vga.putchar(' ');
            }
            vga.putchar('\n');
        }
    }

    return true;
}

// --- atawrite ---

static AtaWriteCommand ataWriteInstance;

AtaWriteCommand::AtaWriteCommand() : Command("atawrite", "Write string to ATA sector") {}

bool AtaWriteCommand::execute(const char** args, usize argc,
                              FileNode*& cwd) {
    VgaTerminal& vga = VgaTerminal::getTerminal();
    AtaPioDriver& ata = AtaPioDriver::getDriver();

    if (argc < 3) {
        vga.print("Usage: atawrite <lba> <string>\n");
        return true;
    }

    if (!ata.isPresent()) {
        vga.print("No ATA drive detected\n");
        return true;
    }

    u32 lba = parse_u32(args[1]);

    // Zero-fill the buffer, then copy the string into it.
    u8 buffer[ATA_SECTOR_SIZE];
    for (u32 i = 0; i < ATA_SECTOR_SIZE; ++i) {
        buffer[i] = 0;
    }

    // Concatenate all remaining args (separated by spaces) into the buffer.
    u32 pos = 0;
    for (usize i = 2; i < argc && pos < ATA_SECTOR_SIZE; ++i) {
        if (i > 2 && pos < ATA_SECTOR_SIZE) {
            buffer[pos++] = ' ';
        }
        for (usize j = 0; args[i][j] != '\0' && pos < ATA_SECTOR_SIZE; ++j) {
            buffer[pos++] = static_cast<u8>(args[i][j]);
        }
    }

    if (!ata.writeSector(lba, buffer)) {
        vga.print("Write error at sector ");
        vga.print_dec(lba);
        vga.putchar('\n');
        return true;
    }

    vga.print("Wrote ");
    vga.print_dec(pos);
    vga.print(" bytes to sector ");
    vga.print_dec(lba);
    vga.putchar('\n');

    return true;
}
