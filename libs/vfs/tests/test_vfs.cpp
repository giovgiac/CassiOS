#include <std/test.hpp>
#include <std/vfs.hpp>

using namespace std;

TEST(vfs_client_size) {
    ASSERT_EQ(sizeof(vfs::Vfs), 4u);
}
