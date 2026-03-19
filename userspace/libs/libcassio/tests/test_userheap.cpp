#include <std/test.hpp>
#include <std/heap.hpp>

using namespace std;

TEST(userheap_alloc_returns_non_null) {
    void* ptr = heap::Heap::alloc(64);
    ASSERT(ptr != nullptr);
    heap::Heap::free(ptr);
}

TEST(userheap_alloc_zero_returns_null) {
    void* ptr = heap::Heap::alloc(0);
    ASSERT(ptr == nullptr);
}

TEST(userheap_alloc_different_addresses) {
    void* a = heap::Heap::alloc(32);
    void* b = heap::Heap::alloc(32);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    ASSERT(a != b);
    heap::Heap::free(b);
    heap::Heap::free(a);
}

TEST(userheap_free_and_realloc) {
    void* a = heap::Heap::alloc(64);
    ASSERT(a != nullptr);
    heap::Heap::free(a);
    void* b = heap::Heap::alloc(64);
    ASSERT(b != nullptr);
    ASSERT_EQ((u32)a, (u32)b);
    heap::Heap::free(b);
}

TEST(userheap_write_and_read) {
    u32* arr = (u32*)heap::Heap::alloc(10 * sizeof(u32));
    ASSERT(arr != nullptr);
    for (u32 i = 0; i < 10; i++) {
        arr[i] = i * 7;
    }
    for (u32 i = 0; i < 10; i++) {
        ASSERT_EQ(arr[i], i * 7);
    }
    heap::Heap::free(arr);
}
