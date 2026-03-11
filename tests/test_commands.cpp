#include <core/commands/command.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::kernel;

TEST(command_registry_has_commands) {
    ASSERT(Command::getCount() > 0);
}

TEST(command_registry_count_matches_expected) {
    // Twelve built-in commands.
    ASSERT_EQ(Command::getCount(), 12);
}

TEST(command_find_existing) {
    Command* cmd = Command::find("help");
    ASSERT(cmd != nullptr);
}

TEST(command_find_nonexistent) {
    Command* cmd = Command::find("nonexistent");
    ASSERT(cmd == nullptr);
}

TEST(command_find_each_builtin) {
    ASSERT(Command::find("help") != nullptr);
    ASSERT(Command::find("clear") != nullptr);
    ASSERT(Command::find("mem") != nullptr);
    ASSERT(Command::find("reboot") != nullptr);
    ASSERT(Command::find("shutdown") != nullptr);
    ASSERT(Command::find("ls") != nullptr);
    ASSERT(Command::find("cd") != nullptr);
    ASSERT(Command::find("pwd") != nullptr);
    ASSERT(Command::find("mkdir") != nullptr);
    ASSERT(Command::find("rmdir") != nullptr);
    ASSERT(Command::find("touch") != nullptr);
    ASSERT(Command::find("rm") != nullptr);
}

TEST(command_name_matches) {
    Command* cmd = Command::find("help");
    ASSERT(cmd != nullptr);

    const char* name = cmd->getName();
    const char* expected = "help";
    u32 i = 0;
    while (name[i] != '\0' && expected[i] != '\0') {
        ASSERT_EQ((u32)name[i], (u32)expected[i]);
        ++i;
    }
    ASSERT_EQ((u32)name[i], (u32)expected[i]);
}

TEST(command_description_not_null) {
    Command* cmd = Command::find("clear");
    ASSERT(cmd != nullptr);
    ASSERT(cmd->getDescription() != nullptr);
}

TEST(command_find_partial_no_match) {
    // "hel" should not match "help".
    Command* cmd = Command::find("hel");
    ASSERT(cmd == nullptr);
}

TEST(command_find_empty_string) {
    Command* cmd = Command::find("");
    ASSERT(cmd == nullptr);
}
