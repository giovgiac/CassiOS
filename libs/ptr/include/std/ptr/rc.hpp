/**
 * ptr/rc.hpp -- Rc<T>: reference-counted shared-ownership smart pointer
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Like Rust's Rc. Single-threaded, non-atomic. Uses a single
 * allocation for the control block and object.
 *
 */

#ifndef STD_PTR_RC_HPP
#define STD_PTR_RC_HPP

#include <std/alloc.hpp>
#include <std/types.hpp>

namespace std {
namespace ptr {

template <typename T> class Rc {
  public:
    // No public constructor from raw pointer -- use Rc::make().

    ~Rc() { release(); }

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

    Rc(Rc&& other) : block(other.block) { other.block = nullptr; }

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

    template <typename... Args> static Rc make(Args&&... args) {
        // Single allocation: control block + object.
        void* mem = operator new(sizeof(ControlBlock));
        ControlBlock* cb = new (mem) ControlBlock(args...);
        return Rc(cb);
    }

  private:
    struct ControlBlock {
        u32 refCount;
        T object;

        template <typename... Args> ControlBlock(Args&&... args) : refCount(1), object(args...) {}
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

} // namespace ptr
} // namespace std

#endif // STD_PTR_RC_HPP
