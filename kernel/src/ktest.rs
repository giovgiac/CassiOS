// ktest.rs -- in-QEMU kernel test harness
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Each module lists its tests with `tests!`; a test returns Err with a
// message when a `check!` or `check_eq!` fails. The report has the same
// [PASS]/[FAIL] lines as the userspace runner; the test kernel's start()
// prints the [DONE] summary. A failure message is kept in a fixed buffer.

use core::fmt::{self, Write};

/// Longest failure message kept; longer ones are cut.
const FAILURE_LEN: usize = 160;

/// Why a test failed.
pub struct Failure {
    text: [u8; FAILURE_LEN],
    len: usize,
}

impl Failure {
    pub fn new(args: fmt::Arguments) -> Self {
        let mut failure = Failure {
            text: [0; FAILURE_LEN],
            len: 0,
        };
        let _ = failure.write_fmt(args);
        failure
    }

    pub fn as_str(&self) -> &str {
        // write_str only stores whole characters.
        core::str::from_utf8(&self.text[..self.len]).unwrap_or_default()
    }
}

impl Write for Failure {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let mut n = s.len().min(FAILURE_LEN - self.len);
        while !s.is_char_boundary(n) {
            n -= 1;
        }
        self.text[self.len..self.len + n].copy_from_slice(&s.as_bytes()[..n]);
        self.len += n;
        Ok(())
    }
}

/// The result of one test.
pub type Outcome = Result<(), Failure>;

pub struct Test {
    pub name: &'static str,
    pub run: fn() -> Outcome,
}

/// Declare a module's `TESTS` list from its test functions.
macro_rules! tests {
    ($($test:ident),* $(,)?) => {
        pub const TESTS: &[$crate::ktest::Test] = &[$(
            $crate::ktest::Test { name: stringify!($test), run: $test },
        )*];
    };
}

/// Fail the test unless `cond` holds.
macro_rules! check {
    ($cond:expr) => {
        if !$cond {
            return Err($crate::ktest::Failure::new(format_args!(
                "assertion failed: \"{}\" at {}:{}",
                stringify!($cond),
                file!(),
                line!()
            )));
        }
    };
}

/// Fail the test unless `left == right`.
macro_rules! check_eq {
    ($left:expr, $right:expr) => {
        let (left, right) = (&$left, &$right);
        if left != right {
            return Err($crate::ktest::Failure::new(format_args!(
                "\"{} == {}\" failed: {:?} != {:?} at {}:{}",
                stringify!($left),
                stringify!($right),
                left,
                right,
                file!(),
                line!()
            )));
        }
    };
}

/// Run every test of every group in order, writing one line per test to
/// `out` and adding to the counts.
pub fn run(groups: &[&[Test]], out: &mut dyn Write, passed: &mut u32, failed: &mut u32) {
    for test in groups.iter().flat_map(|group| group.iter()) {
        let _ = match (test.run)() {
            Ok(()) => {
                *passed += 1;
                writeln!(out, "[PASS] {}", test.name)
            }
            Err(why) => {
                *failed += 1;
                writeln!(out, "[FAIL] {}: {}", test.name, why.as_str())
            }
        };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

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
    fn reports_each_test_and_adds_to_the_counts() {
        let mut out = String::new();
        let (mut passed, mut failed) = (1, 1);
        run(&[TESTS, &TESTS[..1]], &mut out, &mut passed, &mut failed);
        assert_eq!((passed, failed), (3, 3));
        let lines: Vec<&str> = out.lines().collect();
        assert_eq!(lines.len(), 4);
        assert_eq!(lines[0], "[PASS] passes");
        assert!(lines[1].starts_with("[FAIL] fails_check: assertion failed: \"1 > 2\" at "));
        assert!(lines[2].starts_with("[FAIL] fails_check_eq: \"3u32 == 4u32\" failed: 3 != 4 at "));
        assert_eq!(lines[3], "[PASS] passes");
    }

    #[test]
    fn long_failures_are_cut_on_a_character_boundary() {
        let text = "é".repeat(FAILURE_LEN);
        let failure = Failure::new(format_args!("{text}"));
        assert_eq!(failure.as_str(), "é".repeat(FAILURE_LEN / 2));
    }
}
