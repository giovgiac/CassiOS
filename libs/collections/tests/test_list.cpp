#include <std/collections/list.hpp>
#include <std/test.hpp>

using namespace std;

struct Node {
    u32 value;
    Node* next;
    Node(u32 v) : value(v), next(nullptr) {}
};

TEST(list_empty_initial) {
    collections::LinkedList<Node> list;
    ASSERT(list.isEmpty());
    ASSERT_EQ(list.getCount(), (u32)0);
    ASSERT_EQ(reinterpret_cast<usize>(list.getHead()), (usize)0);
}

TEST(list_push_back) {
    collections::LinkedList<Node> list;
    Node a(1), b(2), c(3);
    list.pushBack(&a);
    list.pushBack(&b);
    list.pushBack(&c);
    ASSERT_EQ(list.getCount(), (u32)3);
    ASSERT_EQ(list.getHead()->value, (u32)1);
    ASSERT_EQ(list.getTail()->value, (u32)3);
}

TEST(list_push_front) {
    collections::LinkedList<Node> list;
    Node a(1), b(2);
    list.pushFront(&a);
    list.pushFront(&b);
    ASSERT_EQ(list.getHead()->value, (u32)2);
    ASSERT_EQ(list.getTail()->value, (u32)1);
}

TEST(list_pop_front) {
    collections::LinkedList<Node> list;
    Node a(1), b(2);
    list.pushBack(&a);
    list.pushBack(&b);
    Node* popped = list.popFront();
    ASSERT_EQ(popped->value, (u32)1);
    ASSERT_EQ(list.getCount(), (u32)1);
}

TEST(list_pop_front_empty) {
    collections::LinkedList<Node> list;
    Node* popped = list.popFront();
    ASSERT_EQ(reinterpret_cast<usize>(popped), (usize)0);
}

TEST(list_pop_front_last) {
    collections::LinkedList<Node> list;
    Node a(1);
    list.pushBack(&a);
    list.popFront();
    ASSERT(list.isEmpty());
    ASSERT_EQ(reinterpret_cast<usize>(list.getTail()), (usize)0);
}

TEST(list_remove_middle) {
    collections::LinkedList<Node> list;
    Node a(1), b(2), c(3);
    list.pushBack(&a);
    list.pushBack(&b);
    list.pushBack(&c);
    ASSERT(list.remove(&b));
    ASSERT_EQ(list.getCount(), (u32)2);
    ASSERT_EQ(list.getHead()->next->value, (u32)3);
}

TEST(list_remove_head) {
    collections::LinkedList<Node> list;
    Node a(1), b(2);
    list.pushBack(&a);
    list.pushBack(&b);
    ASSERT(list.remove(&a));
    ASSERT_EQ(list.getHead()->value, (u32)2);
}

TEST(list_remove_tail) {
    collections::LinkedList<Node> list;
    Node a(1), b(2);
    list.pushBack(&a);
    list.pushBack(&b);
    ASSERT(list.remove(&b));
    ASSERT_EQ(list.getTail()->value, (u32)1);
}

TEST(list_remove_not_found) {
    collections::LinkedList<Node> list;
    Node a(1), b(2);
    list.pushBack(&a);
    ASSERT(!list.remove(&b));
    ASSERT_EQ(list.getCount(), (u32)1);
}
