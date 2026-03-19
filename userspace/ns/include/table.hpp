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

#include <std/types.hpp>
#include <std/collections/list.hpp>
#include <ns.hpp>

namespace cassio {

class NsTable {
public:
    static constexpr std::u32 MAX_NAME_LEN = 16;

    struct Entry {
        char name[MAX_NAME_LEN + 1];
        std::u32 pid;
        Entry* next;
    };

    NsTable();

    std::u32 registerName(const char* name, std::u32 pid);
    std::u32 lookup(const char* name);
    std::u32 listAll(NsEntry* buf, std::u32 maxEntries) const;
    std::u32 count() const;

    NsTable(const NsTable&) = delete;
    NsTable(NsTable&&) = delete;
    NsTable& operator=(const NsTable&) = delete;
    NsTable& operator=(NsTable&&) = delete;

private:
    std::collections::LinkedList<Entry> entries;
};

} // cassio

#endif // NS_TABLE_HPP_
