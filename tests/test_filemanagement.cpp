#include <core/commands/command.hpp>
#include <filesystem/filesystem.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;

// --- mkdir ---

TEST(mkdir_creates_directory) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("mkdir");
    ASSERT(cmd != nullptr);

    const char* args[] = {"mkdir", "mk_test"};
    cmd->execute(args, 2, cwd);

    FileNode* dir = fs.resolve("mk_test", cwd);
    ASSERT(dir != nullptr);
    ASSERT(dir->type == FileNodeType::Directory);

    fs.remove(dir);
}

TEST(mkdir_missing_operand) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("mkdir");
    const char* args[] = {"mkdir"};
    ASSERT(cmd->execute(args, 1, cwd));
}

TEST(mkdir_duplicate_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* dir = fs.createDirectory("mk_dup", cwd);

    Command* cmd = Command::find("mkdir");
    const char* args[] = {"mkdir", "mk_dup"};
    ASSERT(cmd->execute(args, 2, cwd));

    // Should still have only one.
    FileNode* found = fs.resolve("mk_dup", cwd);
    ASSERT_EQ((u32)found, (u32)dir);

    fs.remove(dir);
}

// --- rmdir ---

TEST(rmdir_removes_directory) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    fs.createDirectory("rmd_test", cwd);

    Command* cmd = Command::find("rmdir");
    const char* args[] = {"rmdir", "rmd_test"};
    cmd->execute(args, 2, cwd);

    ASSERT(fs.resolve("rmd_test", cwd) == nullptr);
}

TEST(rmdir_nonempty_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* dir = fs.createDirectory("rmd_nonempty", cwd);
    FileNode* file = fs.createFile("rmd_nonempty/child.txt", cwd);

    Command* cmd = Command::find("rmdir");
    const char* args[] = {"rmdir", "rmd_nonempty"};
    cmd->execute(args, 2, cwd);

    // Directory should still exist.
    ASSERT(fs.resolve("rmd_nonempty", cwd) != nullptr);

    fs.remove(file);
    fs.remove(dir);
}

TEST(rmdir_nonexistent) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("rmdir");
    const char* args[] = {"rmdir", "no_such_dir"};
    ASSERT(cmd->execute(args, 2, cwd));
}

TEST(rmdir_on_file_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("rmd_file.txt", cwd);

    Command* cmd = Command::find("rmdir");
    const char* args[] = {"rmdir", "rmd_file.txt"};
    cmd->execute(args, 2, cwd);

    // File should still exist.
    ASSERT(fs.resolve("rmd_file.txt", cwd) != nullptr);

    fs.remove(file);
}

// --- touch ---

TEST(touch_creates_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("touch");
    const char* args[] = {"touch", "tch_test.txt"};
    cmd->execute(args, 2, cwd);

    FileNode* file = fs.resolve("tch_test.txt", cwd);
    ASSERT(file != nullptr);
    ASSERT(file->type == FileNodeType::File);
    ASSERT_EQ(file->size, 0);

    fs.remove(file);
}

TEST(touch_missing_operand) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("touch");
    const char* args[] = {"touch"};
    ASSERT(cmd->execute(args, 1, cwd));
}

TEST(touch_duplicate_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("tch_dup.txt", cwd);

    Command* cmd = Command::find("touch");
    const char* args[] = {"touch", "tch_dup.txt"};
    ASSERT(cmd->execute(args, 2, cwd));

    fs.remove(file);
}

// --- rm ---

TEST(rm_removes_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    fs.createFile("rm_test.txt", cwd);

    Command* cmd = Command::find("rm");
    const char* args[] = {"rm", "rm_test.txt"};
    cmd->execute(args, 2, cwd);

    ASSERT(fs.resolve("rm_test.txt", cwd) == nullptr);
}

TEST(rm_nonexistent) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("rm");
    const char* args[] = {"rm", "no_such_file"};
    ASSERT(cmd->execute(args, 2, cwd));
}

TEST(rm_on_directory_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* dir = fs.createDirectory("rm_dir", cwd);

    Command* cmd = Command::find("rm");
    const char* args[] = {"rm", "rm_dir"};
    cmd->execute(args, 2, cwd);

    // Directory should still exist.
    ASSERT(fs.resolve("rm_dir", cwd) != nullptr);

    fs.remove(dir);
}

TEST(rm_missing_operand) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("rm");
    const char* args[] = {"rm"};
    ASSERT(cmd->execute(args, 1, cwd));
}
