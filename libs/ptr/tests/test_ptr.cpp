#include <std/ptr.hpp>
#include <std/test.hpp>

using namespace std;
using ptr::Box;
using ptr::Rc;

// -- Box<T> --

TEST(box_default_null) {
    Box<u32> b;
    ASSERT(!b);
}

TEST(box_from_raw) {
    Box<u32> b(new u32(42));
    ASSERT(b);
    ASSERT_EQ(*b, (u32)42);
}

TEST(box_make) {
    auto b = Box<u32>::make(7);
    ASSERT_EQ(*b, (u32)7);
}

TEST(box_move_construct) {
    Box<u32> a(new u32(10));
    Box<u32> b(ptr::move(a));
    ASSERT(!a);
    ASSERT(b);
    ASSERT_EQ(*b, (u32)10);
}

TEST(box_move_assign) {
    Box<u32> a(new u32(1));
    Box<u32> b(new u32(2));
    b = ptr::move(a);
    ASSERT(!a);
    ASSERT_EQ(*b, (u32)1);
}

TEST(box_release) {
    Box<u32> b(new u32(99));
    u32* raw = b.release();
    ASSERT(!b);
    ASSERT_EQ(*raw, (u32)99);
    delete raw;
}

TEST(box_arrow) {
    struct S {
        u32 val;
    };
    Box<S> b(new S{42});
    ASSERT_EQ(b->val, (u32)42);
}

// -- Box<T[]> --

TEST(box_array_basic) {
    Box<u32[]> b(new u32[3]);
    b[0] = 10;
    b[1] = 20;
    b[2] = 30;
    ASSERT_EQ(b[0], (u32)10);
    ASSERT_EQ(b[1], (u32)20);
    ASSERT_EQ(b[2], (u32)30);
}

TEST(box_array_move) {
    Box<u32[]> a(new u32[2]);
    a[0] = 5;
    Box<u32[]> b(ptr::move(a));
    ASSERT(!a);
    ASSERT_EQ(b[0], (u32)5);
}

// -- Rc<T> --

TEST(rc_make) {
    auto r = Rc<u32>::make(42);
    ASSERT_EQ(*r, (u32)42);
    ASSERT_EQ(r.refCount(), (u32)1);
}

TEST(rc_copy_increments) {
    auto a = Rc<u32>::make(7);
    {
        Rc<u32> b(a);
        ASSERT_EQ(a.refCount(), (u32)2);
        ASSERT_EQ(b.refCount(), (u32)2);
        ASSERT_EQ(*b, (u32)7);
    }
    ASSERT_EQ(a.refCount(), (u32)1);
}

TEST(rc_copy_assign) {
    auto a = Rc<u32>::make(1);
    auto b = Rc<u32>::make(2);
    b = a;
    ASSERT_EQ(a.refCount(), (u32)2);
    ASSERT_EQ(*b, (u32)1);
}

TEST(rc_move_no_refcount_change) {
    auto a = Rc<u32>::make(10);
    auto b = Rc<u32>(ptr::move(a));
    ASSERT_EQ(b.refCount(), (u32)1);
    ASSERT(!a);
}

TEST(rc_arrow) {
    struct S {
        u32 val;
    };
    auto r = Rc<S>::make(S{42});
    ASSERT_EQ(r->val, (u32)42);
}
