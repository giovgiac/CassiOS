// harness.rs -- test registration, assertions and reporting
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Each module lists its tests with `tests!`; a test returns Err with a
// message when a `check!` or `check_eq!` fails. The report has the same
// [PASS]/[FAIL]/[DONE] lines as the kernel test runner.

use alloc::format;
use alloc::string::String;

/// The result of one test: Err holds why it failed.
pub type Outcome = Result<(), String>;

pub struct Test {
    pub name: &'static str,
    pub run: fn() -> Outcome,
}

/// Declare a module's `TESTS` list from its test functions.
macro_rules! tests {
    ($($test:ident),* $(,)?) => {
        pub const TESTS: &[$crate::harness::Test] = &[$(
            $crate::harness::Test { name: stringify!($test), run: $test },
        )*];
    };
}

/// Fail the test unless `cond` holds.
macro_rules! check {
    ($cond:expr) => {
        if !$cond {
            return Err(alloc::format!(
                "assertion failed: \"{}\" at {}:{}",
                stringify!($cond),
                file!(),
                line!()
            ));
        }
    };
}

/// Fail the test unless `left == right`.
macro_rules! check_eq {
    ($left:expr, $right:expr) => {
        let (left, right) = (&$left, &$right);
        if left != right {
            return Err(alloc::format!(
                "\"{} == {}\" failed: {:?} != {:?} at {}:{}",
                stringify!($left),
                stringify!($right),
                left,
                right,
                file!(),
                line!()
            ));
        }
    };
}

/// Run every test of every group in order, writing one line per test and
/// a summary through `write`. Returns how many failed.
pub fn run(groups: &[&[Test]], write: &mut dyn FnMut(&str)) -> u32 {
    let (mut passed, mut failed) = (0, 0);
    for test in groups.iter().flat_map(|group| group.iter()) {
        match (test.run)() {
            Ok(()) => {
                passed += 1;
                write(&format!("[PASS] {}\n", test.name));
            }
            Err(why) => {
                failed += 1;
                write(&format!("[FAIL] {}: {}\n", test.name, why));
            }
        }
    }
    write(&format!("[DONE] {passed} passed, {failed} failed\n"));
    failed
}

#[cfg(test)]
mod tests {
    use super::*;
    use alloc::vec::Vec;

    fn passes() -> Outcome {
        check!(1 + 1 == 2);
        check_eq!(2 * 2, 4);
        Ok(())
    }

    fn fails_check() -> Outcome {
        check!(1 > 2);
        Ok(())
    }

    fn fails_check_eq() -> Outcome {
        check_eq!(3u32, 4u32);
        Ok(())
    }

    tests!(passes, fails_check, fails_check_eq);

    #[test]
    fn reports_each_test_and_a_summary() {
        let mut out = String::new();
        let failed = run(&[TESTS, &TESTS[..1]], &mut |s| out.push_str(s));
        assert_eq!(failed, 2);
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines[0], "[PASS] passes");
        assert!(lines[1].starts_with("[FAIL] fails_check: assertion failed: \"1 > 2\" at "));
        assert!(lines[2].starts_with("[FAIL] fails_check_eq: \"3u32 == 4u32\" failed: 3 != 4 at "));
        assert_eq!(lines[3], "[PASS] passes");
        assert_eq!(lines[4], "[DONE] 2 passed, 2 failed");
    }
}
