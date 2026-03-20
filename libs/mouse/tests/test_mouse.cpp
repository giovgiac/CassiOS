#include <std/mouse.hpp>
#include <std/test.hpp>

using namespace std;

TEST(mouse_client_size) {
    ASSERT_EQ(sizeof(mouse::Mouse), 4u);
}
