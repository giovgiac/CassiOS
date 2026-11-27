// display.rs -- display service tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::ipc::{self, Message, message_type};
use cassio::ns;

use crate::harness::Outcome;

tests!(display_service_registered, display_get_info_responds);

fn display_service_registered() -> Outcome {
    check!(ns::lookup(b"display") != 0);
    Ok(())
}

fn display_get_info_responds() -> Outcome {
    let pid = ns::lookup(b"display");
    check!(pid != 0);
    let mut msg = Message {
        kind: message_type::DISPLAY_GET_INFO,
        ..Message::default()
    };
    // The dimensions depend on the boot framebuffer, which the test boot
    // (QEMU -kernel) does not set up, so only the reply is checked.
    check_eq!(ipc::send(pid, &mut msg, &[]), 0);
    Ok(())
}
