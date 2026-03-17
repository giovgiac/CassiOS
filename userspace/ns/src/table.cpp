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
#include <userheap.hpp>

using namespace cassio;

NsTable::NsTable() : head(nullptr), entryCount(0) {}

u32 NsTable::registerName(const char* name, u32 pid) {
    if (lookup(name) != 0) {
        return 0;
    }

    void* mem = UserHeap::alloc(sizeof(Entry));
    if (!mem) {
        return 0;
    }

    Entry* entry = (Entry*)mem;
    strcpy(entry->name, name, MAX_NAME_LEN + 1);
    entry->pid = pid;
    entry->next = head;
    head = entry;
    entryCount++;
    return 1;
}

u32 NsTable::lookup(const char* name) {
    Entry* e = head;
    while (e) {
        if (streq(e->name, name)) {
            return e->pid;
        }
        e = e->next;
    }
    return 0;
}

u32 NsTable::count() const {
    return entryCount;
}
