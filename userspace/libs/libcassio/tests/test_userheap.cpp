#include <test.hpp>
#include <userheap.hpp>

using namespace cassio;
using namespace std;

TEST(userheap_alloc_returns_non_null) {
    void* ptr = UserHeap::alloc(64);
    ASSERT(ptr != nullptr);
    UserHeap::free(ptr);
}

TEST(userheap_alloc_zero_returns_null) {
    void* ptr = UserHeap::alloc(0);
    ASSERT(ptr == nullptr);
}

TEST(userheap_alloc_different_addresses) {
    void* a = UserHeap::alloc(32);
    void* b = UserHeap::alloc(32);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    ASSERT(a != b);
    UserHeap::free(b);
    UserHeap::free(a);
}

TEST(userheap_free_and_realloc) {
    void* a = UserHeap::alloc(64);
    ASSERT(a != nullptr);
    UserHeap::free(a);
    void* b = UserHeap::alloc(64);
    ASSERT(b != nullptr);
    ASSERT_EQ((u32)a, (u32)b);
    UserHeap::free(b);
}

TEST(userheap_write_and_read) {
    u32* arr = (u32*)UserHeap::alloc(10 * sizeof(u32));
    ASSERT(arr != nullptr);
    for (u32 i = 0; i < 10; i++) {
        arr[i] = i * 7;
    }
    for (u32 i = 0; i < 10; i++) {
        ASSERT_EQ(arr[i], i * 7);
    }
    UserHeap::free(arr);
}
