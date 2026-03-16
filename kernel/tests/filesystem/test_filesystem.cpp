#include <filesystem/filesystem.hpp>
#include <test.hpp>

using namespace cassio;
using namespace cassio::filesystem;

// --- Root ---

TEST(fs_root_exists) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    ASSERT(root != nullptr);
    ASSERT(root->type == FileNodeType::Directory);
}

TEST(fs_root_parent_is_itself) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    ASSERT_EQ((u32)root->parent, (u32)root);
}

// --- Path Resolution ---

TEST(fs_resolve_root) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* result = fs.resolve("/", root);
    ASSERT_EQ((u32)result, (u32)root);
}

TEST(fs_resolve_dot) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* result = fs.resolve(".", root);
    ASSERT_EQ((u32)result, (u32)root);
}

TEST(fs_resolve_dotdot_at_root) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* result = fs.resolve("..", root);
    ASSERT_EQ((u32)result, (u32)root);
}

TEST(fs_resolve_nonexistent) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* result = fs.resolve("does_not_exist", root);
    ASSERT(result == nullptr);
}

TEST(fs_resolve_null_path) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    ASSERT(fs.resolve(nullptr, root) == nullptr);
}

TEST(fs_resolve_empty_path) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    ASSERT(fs.resolve("", root) == nullptr);
}

// --- Create File ---

TEST(fs_create_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("test_create_file.txt", root);
    ASSERT(file != nullptr);
    ASSERT(file->type == FileNodeType::File);
    ASSERT(file->data == nullptr);
    ASSERT_EQ(file->size, 0);
    fs.remove(file);
}

TEST(fs_create_file_duplicate) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* f1 = fs.createFile("dup_test.txt", root);
    ASSERT(f1 != nullptr);
    FileNode* f2 = fs.createFile("dup_test.txt", root);
    ASSERT(f2 == nullptr);
    fs.remove(f1);
}

TEST(fs_create_file_resolve) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("resolve_test.txt", root);
    ASSERT(file != nullptr);
    FileNode* found = fs.resolve("resolve_test.txt", root);
    ASSERT_EQ((u32)found, (u32)file);
    fs.remove(file);
}

// --- Create Directory ---

TEST(fs_create_directory) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("test_dir", root);
    ASSERT(dir != nullptr);
    ASSERT(dir->type == FileNodeType::Directory);
    fs.remove(dir);
}

TEST(fs_create_nested_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("nest_dir", root);
    ASSERT(dir != nullptr);
    FileNode* file = fs.createFile("nest_dir/nested.txt", root);
    ASSERT(file != nullptr);
    ASSERT_EQ((u32)file->parent, (u32)dir);
    fs.remove(file);
    fs.remove(dir);
}

TEST(fs_resolve_absolute_nested) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("abs_dir", root);
    FileNode* file = fs.createFile("abs_dir/abs_file.txt", root);
    ASSERT(file != nullptr);
    FileNode* found = fs.resolve("/abs_dir/abs_file.txt", root);
    ASSERT_EQ((u32)found, (u32)file);
    fs.remove(file);
    fs.remove(dir);
}

TEST(fs_resolve_relative_from_subdir) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("rel_dir", root);
    FileNode* file = fs.createFile("rel_dir/rel_file.txt", root);
    // Resolve relative path from the subdirectory.
    FileNode* found = fs.resolve("rel_file.txt", dir);
    ASSERT_EQ((u32)found, (u32)file);
    fs.remove(file);
    fs.remove(dir);
}

TEST(fs_resolve_dotdot_from_subdir) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("dotdot_dir", root);
    FileNode* result = fs.resolve("..", dir);
    ASSERT_EQ((u32)result, (u32)root);
    fs.remove(dir);
}

// --- Remove ---

TEST(fs_remove_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("rm_file.txt", root);
    ASSERT(file != nullptr);
    ASSERT(fs.remove(file));
    ASSERT(fs.resolve("rm_file.txt", root) == nullptr);
}

TEST(fs_remove_empty_directory) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("rm_dir", root);
    ASSERT(dir != nullptr);
    ASSERT(fs.remove(dir));
    ASSERT(fs.resolve("rm_dir", root) == nullptr);
}

TEST(fs_remove_nonempty_directory_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("nonempty_dir", root);
    FileNode* file = fs.createFile("nonempty_dir/child.txt", root);
    ASSERT(!fs.remove(dir));
    fs.remove(file);
    fs.remove(dir);
}

TEST(fs_remove_root_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    ASSERT(!fs.remove(fs.getRoot()));
}

// --- Write / Read ---

TEST(fs_write_and_read) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("rw_file.txt", root);
    ASSERT(file != nullptr);

    const u8 data[] = {'H', 'e', 'l', 'l', 'o'};
    ASSERT(fs.write(file, data, 5));
    ASSERT_EQ(file->size, 5);
    ASSERT(file->data != nullptr);
    ASSERT_EQ(file->data[0], 'H');
    ASSERT_EQ(file->data[4], 'o');

    fs.remove(file);
}

TEST(fs_write_overwrites) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("overwrite.txt", root);

    const u8 data1[] = {'A', 'B', 'C'};
    fs.write(file, data1, 3);
    ASSERT_EQ(file->size, 3);

    const u8 data2[] = {'X', 'Y'};
    fs.write(file, data2, 2);
    ASSERT_EQ(file->size, 2);
    ASSERT_EQ(file->data[0], 'X');
    ASSERT_EQ(file->data[1], 'Y');

    fs.remove(file);
}

TEST(fs_write_to_directory_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    const u8 data[] = {'X'};
    ASSERT(!fs.write(root, data, 1));
}

// --- Move ---

TEST(fs_move_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("mv_src.txt", root);
    FileNode* dir = fs.createDirectory("mv_dst", root);

    ASSERT(fs.move(file, "mv_dst/mv_src.txt", root));
    ASSERT(fs.resolve("mv_src.txt", root) == nullptr);
    ASSERT_EQ((u32)fs.resolve("mv_dst/mv_src.txt", root), (u32)file);

    fs.remove(file);
    fs.remove(dir);
}

TEST(fs_move_rename) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("old_name.txt", root);

    ASSERT(fs.move(file, "new_name.txt", root));
    ASSERT(fs.resolve("old_name.txt", root) == nullptr);
    ASSERT_EQ((u32)fs.resolve("new_name.txt", root), (u32)file);

    fs.remove(file);
}

TEST(fs_move_root_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    ASSERT(!fs.move(fs.getRoot(), "somewhere", fs.getRoot()));
}

// --- Copy ---

TEST(fs_copy_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("cp_src.txt", root);
    const u8 data[] = {'C', 'O', 'P', 'Y'};
    fs.write(file, data, 4);

    FileNode* copy = fs.copy(file, "cp_dst.txt", root);
    ASSERT(copy != nullptr);
    ASSERT_EQ(copy->size, 4);
    ASSERT_EQ(copy->data[0], 'C');
    ASSERT_EQ(copy->data[3], 'Y');
    // Verify it's a deep copy (different buffer).
    ASSERT(copy->data != file->data);

    fs.remove(copy);
    fs.remove(file);
}

TEST(fs_copy_directory_fails) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* dir = fs.createDirectory("cp_dir", root);
    FileNode* result = fs.copy(dir, "cp_dir2", root);
    ASSERT(result == nullptr);
    fs.remove(dir);
}

TEST(fs_copy_empty_file) {
    Filesystem& fs = Filesystem::getFilesystem();
    FileNode* root = fs.getRoot();
    FileNode* file = fs.createFile("cp_empty.txt", root);
    FileNode* copy = fs.copy(file, "cp_empty2.txt", root);
    ASSERT(copy != nullptr);
    ASSERT_EQ(copy->size, 0);
    ASSERT(copy->data == nullptr);
    fs.remove(copy);
    fs.remove(file);
}
