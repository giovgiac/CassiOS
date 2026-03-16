#include <core/commands/command.hpp>
#include <filesystem/filesystem.hpp>
#include <string.hpp>
#include <test.hpp>

using namespace cassio;
using namespace cassio::kernel;
using namespace cassio::filesystem;

// --- mv ---

TEST(mv_renames_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("mv_src.txt", cwd);

    Command* cmd = Command::find("mv");
    ASSERT(cmd != nullptr);

    const char* args[] = {"mv", "mv_src.txt", "mv_dst.txt"};
    cmd->execute(args, 3, cwd);

    ASSERT(fs.resolve("mv_src.txt", cwd) == nullptr);
    FileNode* moved = fs.resolve("mv_dst.txt", cwd);
    ASSERT(moved != nullptr);
    ASSERT_EQ((u32)moved, (u32)file);

    fs.remove(moved);
}

TEST(mv_moves_file_to_directory) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("mv_file.txt", cwd);
    FileNode* dir = fs.createDirectory("mv_dest_dir", cwd);

    const u8 data[] = {'a', 'b', 'c'};
    fs.write(file, data, 3);

    Command* cmd = Command::find("mv");
    const char* args[] = {"mv", "mv_file.txt", "mv_dest_dir/mv_file.txt"};
    cmd->execute(args, 3, cwd);

    ASSERT(fs.resolve("mv_file.txt", cwd) == nullptr);
    FileNode* moved = fs.resolve("mv_dest_dir/mv_file.txt", cwd);
    ASSERT(moved != nullptr);
    ASSERT_EQ(moved->size, 3);

    fs.remove(moved);
    fs.remove(dir);
}

TEST(mv_renames_directory) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    fs.createDirectory("mv_old_dir", cwd);

    Command* cmd = Command::find("mv");
    const char* args[] = {"mv", "mv_old_dir", "mv_new_dir"};
    cmd->execute(args, 3, cwd);

    ASSERT(fs.resolve("mv_old_dir", cwd) == nullptr);
    FileNode* moved = fs.resolve("mv_new_dir", cwd);
    ASSERT(moved != nullptr);
    ASSERT(moved->type == FileNodeType::Directory);

    fs.remove(moved);
}

TEST(mv_missing_args) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("mv");
    const char* args1[] = {"mv"};
    ASSERT(cmd->execute(args1, 1, cwd));

    const char* args2[] = {"mv", "something"};
    ASSERT(cmd->execute(args2, 2, cwd));
}

TEST(mv_nonexistent_source) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("mv");
    const char* args[] = {"mv", "no_such.txt", "dest.txt"};
    ASSERT(cmd->execute(args, 3, cwd));
}

// --- cp ---

TEST(cp_copies_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("cp_src.txt", cwd);
    const u8 data[] = {'x', 'y', 'z'};
    fs.write(file, data, 3);

    Command* cmd = Command::find("cp");
    ASSERT(cmd != nullptr);

    const char* args[] = {"cp", "cp_src.txt", "cp_dst.txt"};
    cmd->execute(args, 3, cwd);

    // Source still exists.
    FileNode* src = fs.resolve("cp_src.txt", cwd);
    ASSERT(src != nullptr);
    ASSERT_EQ(src->size, 3);

    // Destination is a separate copy.
    FileNode* dst = fs.resolve("cp_dst.txt", cwd);
    ASSERT(dst != nullptr);
    ASSERT_EQ(dst->size, 3);
    ASSERT_EQ((u32)dst->data[0], (u32)'x');
    ASSERT((u32)dst != (u32)src);

    fs.remove(src);
    fs.remove(dst);
}

TEST(cp_copies_empty_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* file = fs.createFile("cp_empty.txt", cwd);

    Command* cmd = Command::find("cp");
    const char* args[] = {"cp", "cp_empty.txt", "cp_empty_dst.txt"};
    cmd->execute(args, 3, cwd);

    FileNode* dst = fs.resolve("cp_empty_dst.txt", cwd);
    ASSERT(dst != nullptr);
    ASSERT_EQ(dst->size, 0);

    fs.remove(file);
    fs.remove(dst);
}

TEST(cp_directory_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();
    FileNode* dir = fs.createDirectory("cp_dir", cwd);

    Command* cmd = Command::find("cp");
    const char* args[] = {"cp", "cp_dir", "cp_dir_copy"};
    ASSERT(cmd->execute(args, 3, cwd));

    // No copy should exist.
    ASSERT(fs.resolve("cp_dir_copy", cwd) == nullptr);

    fs.remove(dir);
}

TEST(cp_missing_args) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("cp");
    const char* args1[] = {"cp"};
    ASSERT(cmd->execute(args1, 1, cwd));

    const char* args2[] = {"cp", "something"};
    ASSERT(cmd->execute(args2, 2, cwd));
}

TEST(cp_nonexistent_source) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* cwd = fs.getRoot();

    Command* cmd = Command::find("cp");
    const char* args[] = {"cp", "no_such.txt", "dest.txt"};
    ASSERT(cmd->execute(args, 3, cwd));
}
