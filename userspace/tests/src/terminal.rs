// terminal.rs -- terminal service tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The test boot has no framebuffer, so the terminal has no character grid
// and output does not move the cursor; these check that requests are
// accepted and that the cursor requests round-trip.

use cassio::ns;
use cassio::terminal::Terminal;

use crate::harness::Outcome;

tests!(
    terminal_service_registered,
    terminal_output_is_accepted,
    terminal_set_and_get_cursor,
    terminal_clear_resets_cursor,
);

fn terminal_service_registered() -> Outcome {
    check!(ns::lookup(b"terminal") != 0);
    Ok(())
}

fn terminal_output_is_accepted() -> Outcome {
    let terminal = Terminal::connect();
    terminal.putchar(b'A');
    terminal.write(b"test");
    terminal.flush();
    // The cursor request is answered after the output before it.
    terminal.set_cursor(1, 2);
    check_eq!(terminal.cursor(), (1, 2));
    Ok(())
}

fn terminal_set_and_get_cursor() -> Outcome {
    let terminal = Terminal::connect();
    terminal.set_cursor(5, 3);
    check_eq!(terminal.cursor(), (5, 3));
    terminal.set_cursor(10, 7);
    check_eq!(terminal.cursor(), (10, 7));
    Ok(())
}

fn terminal_clear_resets_cursor() -> Outcome {
    let terminal = Terminal::connect();
    terminal.set_cursor(10, 5);
    terminal.clear();
    check_eq!(terminal.cursor(), (0, 0));
    Ok(())
}
