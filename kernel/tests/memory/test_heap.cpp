#include <std/test.hpp>

#include <memory/heap.hpp>

using namespace cassio;
using namespace std;
using namespace cassio::memory;

TEST(heap_allocate_returns_non_null) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    void* ptr = heap.allocate(64);
    ASSERT(ptr != nullptr);
    heap.free(ptr);
}

TEST(heap_allocate_zero_returns_null) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    void* ptr = heap.allocate(0);
    ASSERT(ptr == nullptr);
}

TEST(heap_allocate_different_addresses) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(32);
    void* b = heap.allocate(32);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    ASSERT(a != b);
    heap.free(b);
    heap.free(a);
}

TEST(heap_free_and_realloc) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(64);
    ASSERT(a != nullptr);
    heap.free(a);
    void* b = heap.allocate(64);
    ASSERT(b != nullptr);
    ASSERT_EQ((u32)a, (u32)b);
    heap.free(b);
}

TEST(heap_coalesce_adjacent_free) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(64);
    void* b = heap.allocate(64);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    heap.free(a);
    heap.free(b);
    // After coalescing, should be able to allocate a larger block.
    void* c = heap.allocate(128 + sizeof(alloc::BlockHeader));
    ASSERT(c != nullptr);
    heap.free(c);
}

TEST(heap_split_block) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(32);
    void* b = heap.allocate(32);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    // Second allocation should immediately follow the first.
    u32 expected = (u32)a + 32 + sizeof(alloc::BlockHeader);
    ASSERT_EQ(expected, (u32)b);
    heap.free(b);
    heap.free(a);
}

TEST(heap_large_allocation) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
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

TEST(heap_double_free_does_not_corrupt) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    void* a = heap.allocate(64);
    ASSERT(a != nullptr);
    heap.free(a);
    // Double-free should be silently ignored (block is already free).
    heap.free(a);
    // Heap should still be functional.
    void* b = heap.allocate(64);
    ASSERT(b != nullptr);
    heap.free(b);
}

TEST(heap_free_null_is_safe) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    heap.free(nullptr);
    // Should not crash. Heap still works.
    void* p = heap.allocate(32);
    ASSERT(p != nullptr);
    heap.free(p);
}

TEST(heap_free_out_of_range_is_ignored) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    // Free a pointer that is clearly outside the heap region.
    u8 stack_var = 0;
    heap.free(&stack_var);
    // Heap should still be functional.
    void* p = heap.allocate(32);
    ASSERT(p != nullptr);
    heap.free(p);
}

TEST(heap_extend_too_small_is_ignored) {
    alloc::HeapAllocator& heap = KernelHeap::getAllocator();
    // Extending by less than sizeof(BlockHeader) should be silently ignored
    // (not wrap around to a huge value).
    heap.extend(4);
    // Heap should still be functional.
    void* p = heap.allocate(32);
    ASSERT(p != nullptr);
    heap.free(p);
}
