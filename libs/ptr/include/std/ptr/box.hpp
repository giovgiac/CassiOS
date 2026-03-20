/**
 * ptr/box.hpp -- Box<T>: unique-ownership smart pointer
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Like Rust's Box / C++ unique_ptr. Frees via operator delete
 * on destruction. Array specialization Box<T[]> uses delete[].
 *
 */

#ifndef STD_PTR_BOX_HPP
#define STD_PTR_BOX_HPP

#include <std/types.hpp>

namespace std {
namespace ptr {

// -- Utility: move --

template <typename T> T&& move(T& t) {
    return static_cast<T&&>(t);
}

// ============================================================
// Box<T> -- unique ownership
// ============================================================

template <typename T> class Box {
public:
    explicit Box(T* raw = nullptr) : ptr(raw) {}

    ~Box() { delete ptr; }

    // Move.
    Box(Box&& other) : ptr(other.ptr) { other.ptr = nullptr; }

    Box& operator=(Box&& other) {
        if (this != &other) {
            delete ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    // No copy.
    Box(const Box&) = delete;
    Box& operator=(const Box&) = delete;

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    T* get() const { return ptr; }

    T* release() {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    explicit operator bool() const { return ptr != nullptr; }

    template <typename... Args> static Box make(Args&&... args) { return Box(new T(args...)); }

private:
    T* ptr;
};

// ============================================================
// Box<T[]> -- unique ownership of arrays
// ============================================================

template <typename T> class Box<T[]> {
public:
    explicit Box(T* raw = nullptr) : ptr(raw) {}

    ~Box() { delete[] ptr; }

    // Move.
    Box(Box&& other) : ptr(other.ptr) { other.ptr = nullptr; }

    Box& operator=(Box&& other) {
        if (this != &other) {
            delete[] ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    // No copy.
    Box(const Box&) = delete;
    Box& operator=(const Box&) = delete;

    T& operator[](usize i) const { return ptr[i]; }
    T* get() const { return ptr; }

    T* release() {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    explicit operator bool() const { return ptr != nullptr; }

private:
    T* ptr;
};

} // namespace ptr
} // namespace std

#endif // STD_PTR_BOX_HPP
