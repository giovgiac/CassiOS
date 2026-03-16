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

namespace cassio {

class NsTable {
public:
    static constexpr u32 MAX_ENTRIES = 16;
    static constexpr u32 MAX_NAME_LEN = 16;

    NsTable();

    u32 registerName(const char* name, u32 pid);
    u32 lookup(const char* name);
    u32 count() const;

private:
    struct Entry {
        char name[MAX_NAME_LEN + 1];
        u32 pid;
    };

    Entry entries[MAX_ENTRIES];
    u32 entryCount;
};

} // cassio

#endif // NS_TABLE_HPP_
