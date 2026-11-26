// mouse.rs -- mouse service tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// No Rust program reads the mouse yet, so these talk to the service with
// raw messages.

use cassio::ipc::{self, Message, message_type};
use cassio::ns;

use crate::harness::Outcome;

tests!(mouse_service_registered, mouse_read_state);

fn mouse_service_registered() -> Outcome {
    check!(ns::lookup(b"mouse") != 0);
    Ok(())
}

fn mouse_read_state() -> Outcome {
    let pid = ns::lookup(b"mouse");
    check!(pid != 0);
    let mut msg = Message {
        kind: message_type::MOUSE_READ,
        ..Message::default()
    };
    check_eq!(ipc::send(pid, &mut msg, &[]), 0);
    // Nothing moves the mouse in QEMU: no buttons, no movement.
    check_eq!((msg.arg1, msg.arg2, msg.arg3), (0, 0, 0));
    Ok(())
}
