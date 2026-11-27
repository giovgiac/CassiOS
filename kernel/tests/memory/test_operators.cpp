#include <std/test.hpp>

#include <memory/heap.hpp>

using namespace std;

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
