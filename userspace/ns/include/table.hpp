/**
 * table.hpp -- nameserver lookup table
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef NS_TABLE_HPP_
#define NS_TABLE_HPP_

#include <types.hpp>
#include <list.hpp>

namespace cassio {

class NsTable {
public:
    static constexpr u32 MAX_NAME_LEN = 16;

    struct Entry {
        char name[MAX_NAME_LEN + 1];
        u32 pid;
        Entry* next;
    };

    NsTable();

    u32 registerName(const char* name, u32 pid);
    u32 lookup(const char* name);
    u32 count() const;

    NsTable(const NsTable&) = delete;
    NsTable& operator=(const NsTable&) = delete;

private:
    LinkedList<Entry> entries;
};

} // cassio

#endif // NS_TABLE_HPP_
