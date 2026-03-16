#include <memory/heap.hpp>
#include <test.hpp>

using namespace cassio;
using namespace cassio::memory;

TEST(heap_allocate_returns_non_null) {
    HeapAllocator& heap = KernelHeap::getAllocator();
    void* ptr = heap.allocate(64);
    ASSERT(ptr != nullptr);
    heap.free(ptr);
}

TEST(heap_allocate_zero_returns_null) {
    HeapAllocator& heap = KernelHeap::getAllocator();
    void* ptr = heap.allocate(0);
    ASSERT(ptr == nullptr);
}

TEST(heap_allocate_different_addresses) {
    HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(32);
    void* b = heap.allocate(32);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    ASSERT(a != b);
    heap.free(b);
    heap.free(a);
}

TEST(heap_free_and_realloc) {
    HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(64);
    ASSERT(a != nullptr);
    heap.free(a);
    void* b = heap.allocate(64);
    ASSERT(b != nullptr);
    ASSERT_EQ((u32)a, (u32)b);
    heap.free(b);
}

TEST(heap_coalesce_adjacent_free) {
    HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(64);
    void* b = heap.allocate(64);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    heap.free(a);
    heap.free(b);
    // After coalescing, should be able to allocate a larger block.
    void* c = heap.allocate(128 + sizeof(BlockHeader));
    ASSERT(c != nullptr);
    heap.free(c);
}

TEST(heap_split_block) {
    HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(32);
    void* b = heap.allocate(32);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    // Second allocation should immediately follow the first.
    u32 expected = (u32)a + 32 + sizeof(BlockHeader);
    ASSERT_EQ(expected, (u32)b);
    heap.free(b);
    heap.free(a);
}

TEST(heap_large_allocation) {
    HeapAllocator& heap = KernelHeap::getAllocator();
    void* ptr = heap.allocate(4096);
    ASSERT(ptr != nullptr);
    heap.free(ptr);
}

TEST(heap_operator_new_delete) {
    int* p = new int;
    ASSERT(p != nullptr);
    *p = 42;
    ASSERT_EQ(*p, 42);
    delete p;
}

TEST(heap_operator_new_array_delete_array) {
    int* arr = new int[10];
    ASSERT(arr != nullptr);
    for (int i = 0; i < 10; i++) {
        arr[i] = i;
    }
    ASSERT_EQ(arr[0], 0);
    ASSERT_EQ(arr[9], 9);
    delete[] arr;
}
