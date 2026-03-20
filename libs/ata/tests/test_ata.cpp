#include <std/ata.hpp>
#include <std/test.hpp>

using namespace std;

TEST(ata_client_size) {
    ASSERT_EQ(sizeof(ata::Ata), 4u);
}

TEST(ata_sector_size_constant) {
    ASSERT_EQ(ata::SECTOR_SIZE, 512u);
}
