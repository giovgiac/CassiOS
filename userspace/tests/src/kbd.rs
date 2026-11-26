// kbd.rs -- keyboard service tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::ns;

use crate::harness::Outcome;

tests!(kbd_service_registered);

fn kbd_service_registered() -> Outcome {
    check!(ns::lookup(b"kbd") != 0);
    Ok(())
}
