/**
 * table.cpp -- nameserver lookup table
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <table.hpp>
#include <std/str.hpp>
#include <std/heap.hpp>

using namespace cassio;
using namespace std;
using str::StringView;

NsTable::NsTable() {}

u32 NsTable::registerName(const char* name, u32 pid) {
    if (lookup(name) != 0) {
        return 0;
    }

    void* mem = heap::alloc(sizeof(Entry));
    if (!mem) {
        return 0;
    }

    Entry* entry = (Entry*)mem;
    StringView(name).copyTo(entry->name, MAX_NAME_LEN + 1);
    entry->pid = pid;
    entries.pushFront(entry);
    return 1;
}

u32 NsTable::lookup(const char* name) {
    StringView target(name);
    for (Entry* e = entries.getHead(); e; e = e->next) {
        if (StringView(e->name) == target) {
            return e->pid;
        }
    }
    return 0;
}

u32 NsTable::listAll(ns::Entry* buf, u32 maxEntries) const {
    u32 count = 0;
    for (Entry* e = entries.getHead(); e && count < maxEntries; e = e->next) {
        StringView(e->name).copyTo(buf[count].name, 20);
        buf[count].pid = e->pid;
        ++count;
    }
    return count;
}

u32 NsTable::count() const {
    return entries.getCount();
}
