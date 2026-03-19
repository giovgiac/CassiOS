#include <std/test.hpp>
#include <std/vga.hpp>

using namespace std;

TEST(vga_client_size) {
    ASSERT_EQ(sizeof(vga::Vga), 4u);
}
