/**
 * test_terminal.cpp -- Terminal client library tests
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <std/terminal.hpp>
#include <std/test.hpp>

using namespace std;

TEST(terminal_client_size) {
    ASSERT_EQ(sizeof(terminal::Terminal), (usize)4);
}
