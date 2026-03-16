#include <core/commands/command.hpp>
#include <filesystem/filesystem.hpp>
#include "test.hpp"

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;

// --- cd ---

TEST(cd_to_root_no_args) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("cd_test1", root);
    FileNode* cwd = dir;

    Command* cmd = Command::find("cd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cd"};
    cmd->execute(args, 1, cwd);
    ASSERT_EQ((u32)cwd, (u32)root);

    fs.remove(dir);
}

TEST(cd_to_directory) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("cd_test2", root);
    FileNode* cwd = root;

    Command* cmd = Command::find("cd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cd", "cd_test2"};
    cmd->execute(args, 2, cwd);
    ASSERT_EQ((u32)cwd, (u32)dir);

    fs.remove(dir);
}

TEST(cd_nonexistent_stays) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* cwd = root;

    Command* cmd = Command::find("cd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cd", "nonexistent"};
    cmd->execute(args, 2, cwd);
    ASSERT_EQ((u32)cwd, (u32)root);
}

TEST(cd_to_file_stays) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("cd_file.txt", root);
    FileNode* cwd = root;

    Command* cmd = Command::find("cd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cd", "cd_file.txt"};
    cmd->execute(args, 2, cwd);
    ASSERT_EQ((u32)cwd, (u32)root);

    fs.remove(file);
}

TEST(cd_dotdot) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("cd_test3", root);
    FileNode* cwd = dir;

    Command* cmd = Command::find("cd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cd", ".."};
    cmd->execute(args, 2, cwd);
    ASSERT_EQ((u32)cwd, (u32)root);

    fs.remove(dir);
}

TEST(cd_absolute_path) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("cd_abs", root);
    FileNode* cwd = root;

    Command* cmd = Command::find("cd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cd", "/cd_abs"};
    cmd->execute(args, 2, cwd);
    ASSERT_EQ((u32)cwd, (u32)dir);

    fs.remove(dir);
}

// --- ls ---

TEST(ls_returns_true) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("ls");
    ASSERT(cmd != nullptr);

    const char* args[] = {"ls"};
    ASSERT(cmd->execute(args, 1, cwd));
}

TEST(ls_nonexistent_returns_true) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("ls");
    ASSERT(cmd != nullptr);

    const char* args[] = {"ls", "nonexistent"};
    ASSERT(cmd->execute(args, 2, cwd));
}

TEST(ls_file_returns_true) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("ls_file.txt", root);
    FileNode* cwd = root;

    Command* cmd = Command::find("ls");
    ASSERT(cmd != nullptr);

    const char* args[] = {"ls", "ls_file.txt"};
    ASSERT(cmd->execute(args, 2, cwd));

    fs.remove(file);
}

// --- pwd ---

TEST(pwd_returns_true) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("pwd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"pwd"};
    ASSERT(cmd->execute(args, 1, cwd));
}

TEST(pwd_does_not_change_cwd) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("pwd_dir", root);
    FileNode* cwd = dir;

    Command* cmd = Command::find("pwd");
    ASSERT(cmd != nullptr);

    const char* args[] = {"pwd"};
    cmd->execute(args, 1, cwd);
    ASSERT_EQ((u32)cwd, (u32)dir);

    fs.remove(dir);
}
