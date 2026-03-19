#include <std/test.hpp>
#include <std/kbd.hpp>

using namespace std;

TEST(kbd_client_size) {
    ASSERT_EQ(sizeof(kbd::Kbd), 4u);
}
