#include <core/commands/command.hpp>
#include <filesystem/filesystem.hpp>
#include <string.hpp>
#include <test.hpp>

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;

// --- cat ---

TEST(cat_prints_file_contents) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("cat_test.txt", cwd);
    const u8 data[] = {'h', 'e', 'l', 'l', 'o'};
    fs.write(file, data, 5);

    Command* cmd = Command::find("cat");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cat", "cat_test.txt"};
    ASSERT(cmd->execute(args, 2, cwd));

    // Verify file still exists and data unchanged.
    ASSERT_EQ(file->size, 5);
    ASSERT_EQ((u32)file->data[0], (u32)'h');

    fs.remove(file);
}

TEST(cat_empty_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("cat_empty.txt", cwd);

    Command* cmd = Command::find("cat");
    const char* args[] = {"cat", "cat_empty.txt"};
    ASSERT(cmd->execute(args, 2, cwd));

    fs.remove(file);
}

TEST(cat_missing_operand) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("cat");
    const char* args[] = {"cat"};
    ASSERT(cmd->execute(args, 1, cwd));
}

TEST(cat_nonexistent_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("cat");
    const char* args[] = {"cat", "no_such_file.txt"};
    ASSERT(cmd->execute(args, 2, cwd));
}

TEST(cat_on_directory_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* dir = fs.createDirectory("cat_dir", cwd);

    Command* cmd = Command::find("cat");
    const char* args[] = {"cat", "cat_dir"};
    ASSERT(cmd->execute(args, 2, cwd));

    fs.remove(dir);
}

// --- write ---

TEST(write_writes_text_to_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("wr_test.txt", cwd);

    Command* cmd = Command::find("write");
    ASSERT(cmd != nullptr);

    const char* args[] = {"write", "wr_test.txt", "hello"};
    cmd->execute(args, 3, cwd);

    ASSERT_EQ(file->size, 5);
    ASSERT_EQ((u32)file->data[0], (u32)'h');
    ASSERT_EQ((u32)file->data[4], (u32)'o');

    fs.remove(file);
}

TEST(write_joins_multiple_args) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("wr_multi.txt", cwd);

    Command* cmd = Command::find("write");
    const char* args[] = {"write", "wr_multi.txt", "hello", "world"};
    cmd->execute(args, 4, cwd);

    // "hello world" = 11 bytes
    ASSERT_EQ(file->size, 11);
    ASSERT_EQ((u32)file->data[5], (u32)' ');
    ASSERT_EQ((u32)file->data[6], (u32)'w');

    fs.remove(file);
}

TEST(write_overwrites_existing_content) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("wr_overwrite.txt", cwd);

    const u8 old_data[] = {'o', 'l', 'd'};
    fs.write(file, old_data, 3);

    Command* cmd = Command::find("write");
    const char* args[] = {"write", "wr_overwrite.txt", "new"};
    cmd->execute(args, 3, cwd);

    ASSERT_EQ(file->size, 3);
    ASSERT_EQ((u32)file->data[0], (u32)'n');

    fs.remove(file);
}

TEST(write_missing_args) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("write");
    const char* args1[] = {"write"};
    ASSERT(cmd->execute(args1, 1, cwd));

    const char* args2[] = {"write", "somefile"};
    ASSERT(cmd->execute(args2, 2, cwd));
}

TEST(write_nonexistent_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("write");
    const char* args[] = {"write", "no_such.txt", "data"};
    ASSERT(cmd->execute(args, 3, cwd));
}

TEST(write_on_directory_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* dir = fs.createDirectory("wr_dir", cwd);

    Command* cmd = Command::find("write");
    const char* args[] = {"write", "wr_dir", "data"};
    ASSERT(cmd->execute(args, 3, cwd));

    fs.remove(dir);
}

// --- echo ---

TEST(echo_no_args_prints_newline) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("echo");
    ASSERT(cmd != nullptr);

    const char* args[] = {"echo"};
    ASSERT(cmd->execute(args, 1, cwd));
}

TEST(echo_prints_text) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("echo");
    const char* args[] = {"echo", "hello", "world"};
    ASSERT(cmd->execute(args, 3, cwd));
}
