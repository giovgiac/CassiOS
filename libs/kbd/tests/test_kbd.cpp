#include <std/kbd.hpp>
#include <std/test.hpp>

using namespace std;

TEST(kbd_client_size) {
    ASSERT_EQ(sizeof(kbd::Kbd), 4u);
}
