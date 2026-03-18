/**
 * test_shell.cpp -- Shell unit tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <shell.hpp>
#include <string.hpp>
#include <test.hpp>

using namespace cassio;
using namespace std;

// --- parseArgs tests ---

TEST(shell_parse_args_single_word) {
    char buf[] = "help";
    const char* args[16];
    u8 argc = Shell::parseArgs(buf, 4, args, 16);
    ASSERT_EQ(static_cast<u32>(argc), 1u);
    ASSERT(streq(args[0], "help"));
}

TEST(shell_parse_args_multiple) {
    char buf[] = "write foo hello";
    const char* args[16];
    u8 argc = Shell::parseArgs(buf, 15, args, 16);
    ASSERT_EQ(static_cast<u32>(argc), 3u);
    ASSERT(streq(args[0], "write"));
    ASSERT(streq(args[1], "foo"));
    ASSERT(streq(args[2], "hello"));
}

TEST(shell_parse_args_leading_spaces) {
    char buf[] = "  ls";
    const char* args[16];
    u8 argc = Shell::parseArgs(buf, 4, args, 16);
    ASSERT_EQ(static_cast<u32>(argc), 1u);
    ASSERT(streq(args[0], "ls"));
}

TEST(shell_parse_args_trailing_spaces) {
    char buf[] = "ls  ";
    const char* args[16];
    u8 argc = Shell::parseArgs(buf, 4, args, 16);
    ASSERT_EQ(static_cast<u32>(argc), 1u);
    ASSERT(streq(args[0], "ls"));
}

TEST(shell_parse_args_empty) {
    char buf[] = "";
    const char* args[16];
    u8 argc = Shell::parseArgs(buf, 0, args, 16);
    ASSERT_EQ(static_cast<u32>(argc), 0u);
}

TEST(shell_parse_args_max_limit) {
    char buf[] = "a b c";
    const char* args[2];
    u8 argc = Shell::parseArgs(buf, 5, args, 2);
    ASSERT_EQ(static_cast<u32>(argc), 2u);
    ASSERT(streq(args[0], "a"));
    ASSERT(streq(args[1], "b"));
}

// --- resolvePath tests ---

TEST(shell_resolve_path_absolute) {
    char out[20];
    Shell::resolvePath("/", "/foo", out, 20);
    ASSERT(streq(out, "/foo"));
}

TEST(shell_resolve_path_relative_from_root) {
    char out[20];
    Shell::resolvePath("/", "foo", out, 20);
    ASSERT(streq(out, "/foo"));
}

TEST(shell_resolve_path_relative_from_subdir) {
    char out[20];
    Shell::resolvePath("/bar", "foo", out, 20);
    ASSERT(streq(out, "/bar/foo"));
}

TEST(shell_resolve_path_absolute_ignores_cwd) {
    char out[20];
    Shell::resolvePath("/bar", "/foo", out, 20);
    ASSERT(streq(out, "/foo"));
}

// --- parentDir tests ---

TEST(shell_parent_dir_root) {
    char path[20] = "/";
    Shell::parentDir(path);
    ASSERT(streq(path, "/"));
}

TEST(shell_parent_dir_one_level) {
    char path[20] = "/foo";
    Shell::parentDir(path);
    ASSERT(streq(path, "/"));
}

TEST(shell_parent_dir_two_levels) {
    char path[20] = "/foo/bar";
    Shell::parentDir(path);
    ASSERT(streq(path, "/foo"));
}
