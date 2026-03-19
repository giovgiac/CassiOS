/**
 * ptr.hpp -- Smart pointer types: Box<T> and Rc<T>
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Box<T>:  Unique-ownership pointer (like Rust Box / C++ unique_ptr).
 * Rc<T>:   Reference-counted shared pointer (like Rust Rc).
 *
 * Header-only -- no .cpp or .a needed.
 *
 */

#ifndef STD_PTR_HPP
#define STD_PTR_HPP

#include <std/types.hpp>
#include <std/alloc.hpp>

namespace std {
namespace ptr {

// -- Utility: move --

template <typename T>
T&& move(T& t) { return static_cast<T&&>(t); }

// ============================================================
// Box<T> -- unique ownership
// ============================================================

template <typename T>
class Box {
public:
    explicit Box(T* raw = nullptr) : ptr(raw) {}

    ~Box() {
        delete ptr;
    }

    // Move.
    Box(Box&& other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }

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

    template <typename... Args>
    static Box make(Args&&... args) {
        return Box(new T(args...));
    }

private:
    T* ptr;
};

// ============================================================
// Box<T[]> -- unique ownership of arrays
// ============================================================

template <typename T>
class Box<T[]> {
public:
    explicit Box(T* raw = nullptr) : ptr(raw) {}

    ~Box() {
        delete[] ptr;
    }

    // Move.
    Box(Box&& other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }

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

// ============================================================
// Rc<T> -- reference-counted shared ownership
// ============================================================

template <typename T>
class Rc {
public:
    // No public constructor from raw pointer -- use Rc::make().

    ~Rc() {
        release();
    }

    Rc(const Rc& other) : block(other.block) {
        if (block) {
            block->refCount++;
        }
    }

    Rc& operator=(const Rc& other) {
        if (this != &other) {
            release();
            block = other.block;
            if (block) {
                block->refCount++;
            }
        }
        return *this;
    }

    Rc(Rc&& other) : block(other.block) {
        other.block = nullptr;
    }

    Rc& operator=(Rc&& other) {
        if (this != &other) {
            release();
            block = other.block;
            other.block = nullptr;
        }
        return *this;
    }

    T& operator*() const { return block->object; }
    T* operator->() const { return &block->object; }
    T* get() const { return block ? &block->object : nullptr; }
    explicit operator bool() const { return block != nullptr; }

    u32 refCount() const { return block ? block->refCount : 0; }

    template <typename... Args>
    static Rc make(Args&&... args) {
        // Single allocation: control block + object.
        void* mem = operator new(sizeof(ControlBlock));
        ControlBlock* cb = new (mem) ControlBlock(args...);
        return Rc(cb);
    }

private:
    struct ControlBlock {
        u32 refCount;
        T object;

        template <typename... Args>
        ControlBlock(Args&&... args) : refCount(1), object(args...) {}
    };

    explicit Rc(ControlBlock* cb) : block(cb) {}

    void release() {
        if (block) {
            block->refCount--;
            if (block->refCount == 0) {
                block->object.~T();
                operator delete(block);
            }
            block = nullptr;
        }
    }

    ControlBlock* block;
};

} // ptr
} // std

#endif // STD_PTR_HPP
