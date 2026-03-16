/**
 * table.cpp -- nameserver lookup table
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <table.hpp>
#include <string.hpp>

using namespace cassio;

NsTable::NsTable() : entryCount(0) {
    for (u32 i = 0; i < MAX_ENTRIES; i++) {
        entries[i].name[0] = '\0';
        entries[i].pid = 0;
    }
}

u32 NsTable::registerName(const char* name, u32 pid) {
    if (entryCount >= MAX_ENTRIES || lookup(name) != 0) {
        return 0;
    }
    strcpy(entries[entryCount].name, name, MAX_NAME_LEN + 1);
    entries[entryCount].pid = pid;
    entryCount++;
    return 1;
}

u32 NsTable::lookup(const char* name) {
    for (u32 i = 0; i < entryCount; i++) {
        if (streq(entries[i].name, name)) {
            return entries[i].pid;
        }
    }
    return 0;
}

u32 NsTable::count() const {
    return entryCount;
}
