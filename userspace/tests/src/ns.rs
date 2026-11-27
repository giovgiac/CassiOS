// ns.rs -- nameserver tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::ns::{self, Entry};

use crate::harness::Outcome;

tests!(
    ns_register_and_lookup,
    ns_lookup_unregistered_returns_zero,
    ns_register_multiple_and_lookup,
    ns_register_duplicate_fails,
    ns_list_all_returns_registered_services,
    ns_list_all_contains_known_service,
);

fn ns_register_and_lookup() -> Outcome {
    check_eq!(ns::register_name(b"test_svc"), 1);
    check!(ns::lookup(b"test_svc") != 0);
    Ok(())
}

fn ns_lookup_unregistered_returns_zero() -> Outcome {
    check_eq!(ns::lookup(b"nonexist"), 0);
    Ok(())
}

fn ns_register_multiple_and_lookup() -> Outcome {
    for name in [&b"svc_a"[..], b"svc_b", b"svc_c"] {
        ns::register_name(name);
    }
    // All registered by this process, so all map to the same PID.
    let a = ns::lookup(b"svc_a");
    check!(a != 0);
    check_eq!(ns::lookup(b"svc_b"), a);
    check_eq!(ns::lookup(b"svc_c"), a);
    Ok(())
}

fn ns_register_duplicate_fails() -> Outcome {
    ns::register_name(b"dup_test");
    check_eq!(ns::register_name(b"dup_test"), 0);
    Ok(())
}

fn ns_list_all_returns_registered_services() -> Outcome {
    let mut entries = [Entry::default(); 16];
    let count = ns::list_all(&mut entries);
    check!(count > 0);
    for entry in &entries[..count] {
        check!(!entry.name().is_empty());
        check!(entry.pid != 0);
    }
    Ok(())
}

fn ns_list_all_contains_known_service() -> Outcome {
    let mut entries = [Entry::default(); 16];
    let count = ns::list_all(&mut entries);
    check!(entries[..count].iter().any(|e| e.name() == b"kbd"));
    Ok(())
}
