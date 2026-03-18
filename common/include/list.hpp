/**
 * list.hpp -- intrusive linked list template
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_LIST_HPP_
#define COMMON_LIST_HPP_

#include <std/types.hpp>

namespace cassio {

/**
 * @brief Intrusive singly-linked list.
 *
 * T must have a public `T* next` member. The list manages head/tail
 * pointers and a count; callers handle node allocation and deallocation.
 *
 */
template <typename T>
class LinkedList {
public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}

    void pushBack(T* node) {
        node->next = nullptr;
        if (tail) {
            tail->next = node;
        } else {
            head = node;
        }
        tail = node;
        count++;
    }

    void pushFront(T* node) {
        node->next = head;
        if (!head) {
            tail = node;
        }
        head = node;
        count++;
    }

    T* popFront() {
        if (!head) {
            return nullptr;
        }
        T* node = head;
        head = head->next;
        if (!head) {
            tail = nullptr;
        }
        count--;
        return node;
    }

    bool remove(T* target) {
        T* prev = nullptr;
        T* p = head;
        while (p) {
            if (p == target) {
                if (prev) {
                    prev->next = p->next;
                } else {
                    head = p->next;
                }
                if (p == tail) {
                    tail = prev;
                }
                count--;
                return true;
            }
            prev = p;
            p = p->next;
        }
        return false;
    }

    T* getHead() const { return head; }
    T* getTail() const { return tail; }
    std::u32 getCount() const { return count; }
    bool isEmpty() const { return head == nullptr; }

private:
    T* head;
    T* tail;
    std::u32 count;
};

} // cassio

#endif // COMMON_LIST_HPP_
