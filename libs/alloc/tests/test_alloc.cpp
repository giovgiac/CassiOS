#include <std/alloc.hpp>
#include <test.hpp>

using namespace std;

static u8 test_region[1024];

TEST(alloc_heap_basic) {
    alloc::HeapAllocator a(test_region, sizeof(test_region));
    void* ptr = a.allocate(64);
    ASSERT(ptr != nullptr);
    a.free(ptr);
}

TEST(alloc_heap_zero_returns_null) {
    alloc::HeapAllocator a(test_region, sizeof(test_region));
    void* ptr = a.allocate(0);
    ASSERT_EQ(reinterpret_cast<usize>(ptr), (usize)0);
}

TEST(alloc_heap_multiple) {
    alloc::HeapAllocator a(test_region, sizeof(test_region));
    void* p1 = a.allocate(32);
    void* p2 = a.allocate(32);
    ASSERT(p1 != nullptr);
    ASSERT(p2 != nullptr);
    ASSERT(p1 != p2);
    a.free(p1);
    a.free(p2);
}

TEST(alloc_heap_reuse_after_free) {
    alloc::HeapAllocator a(test_region, sizeof(test_region));
    void* p1 = a.allocate(64);
    a.free(p1);
    void* p2 = a.allocate(64);
    ASSERT(p2 != nullptr);
    a.free(p2);
}

TEST(alloc_block_header_size) {
    ASSERT_EQ(static_cast<u32>(sizeof(alloc::BlockHeader)), (u32)12);
}
