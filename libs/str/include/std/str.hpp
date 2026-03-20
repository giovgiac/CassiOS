/**
 * str.hpp -- StringView and string utilities
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef STD_STR_HPP
#define STD_STR_HPP

#include <std/types.hpp>

namespace std {
namespace str {

/**
 * @brief Non-owning, non-allocating string view (pointer + length).
 *
 * 8 bytes on i386. Implicit conversion from const char* makes most
 * call sites seamless when migrating from raw C strings.
 *
 */
class StringView {
  public:
    StringView() : ptr(nullptr), len(0) {}
    StringView(const char* s);
    StringView(const char* s, usize length) : ptr(s), len(length) {}

    const char* data() const { return ptr; }
    usize length() const { return len; }
    bool isEmpty() const { return len == 0; }
    char operator[](usize i) const { return ptr[i]; }

    bool operator==(const StringView& other) const;
    bool operator!=(const StringView& other) const;
    i32 compare(const StringView& other) const;

    bool startsWith(const StringView& prefix) const;
    bool endsWith(const StringView& suffix) const;
    bool contains(char c) const;
    isize indexOf(char c) const;

    StringView substr(usize pos) const;
    StringView substr(usize pos, usize count) const;

    void copyTo(char* dst, usize max) const;

    template <typename T> T to() const;

  private:
    const char* ptr;
    usize len;
};

} // namespace str
} // namespace std

#endif // STD_STR_HPP
