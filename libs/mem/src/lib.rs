// lib.rs -- memcpy, memmove, memset, memcmp and bcmp
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The memory functions the compiler calls for copies, fills and
// comparisons, for the kernel and every program (the prebuilt core and
// alloc crates call them too). The kernel and cassio link this crate. It
// is built with no_builtins, or LLVM would turn these loops back into
// calls to themselves. Copies and fills move a word at a time once both
// pointers are word-aligned. The C symbols are only exported in the i586
// build, so host tests do not replace the host's own.

#![no_std]
#![no_builtins]

const WORD: usize = core::mem::size_of::<u32>();

fn misalignment(p: *const u8) -> usize {
    p as usize % WORD
}

/// Copy `n` bytes from `src` to `dst`, front to back: the regions may
/// overlap only if `dst` is below `src`.
///
/// # Safety
///
/// `src` must be readable and `dst` writable for `n` bytes.
pub unsafe fn copy(dst: *mut u8, src: *const u8, n: usize) -> *mut u8 {
    let (mut d, mut s, mut n) = (dst, src, n);
    unsafe {
        if misalignment(d) == misalignment(s) {
            while n > 0 && misalignment(d) != 0 {
                *d = *s;
                (d, s, n) = (d.add(1), s.add(1), n - 1);
            }
            while n >= WORD {
                *(d as *mut u32) = *(s as *const u32);
                (d, s, n) = (d.add(WORD), s.add(WORD), n - WORD);
            }
        }
        while n > 0 {
            *d = *s;
            (d, s, n) = (d.add(1), s.add(1), n - 1);
        }
    }
    dst
}

/// Copy `n` bytes from `src` to `dst`; the regions may overlap.
///
/// # Safety
///
/// As for `copy`.
pub unsafe fn copy_overlapping(dst: *mut u8, src: *const u8, n: usize) -> *mut u8 {
    if (dst as usize) <= (src as usize) {
        return unsafe { copy(dst, src, n) };
    }
    // Back to front, from the ends.
    let (mut d, mut s, mut n) = unsafe { (dst.add(n), src.add(n), n) };
    unsafe {
        if misalignment(d) == misalignment(s) {
            while n > 0 && misalignment(d) != 0 {
                (d, s, n) = (d.sub(1), s.sub(1), n - 1);
                *d = *s;
            }
            while n >= WORD {
                (d, s, n) = (d.sub(WORD), s.sub(WORD), n - WORD);
                *(d as *mut u32) = *(s as *const u32);
            }
        }
        while n > 0 {
            (d, s, n) = (d.sub(1), s.sub(1), n - 1);
            *d = *s;
        }
    }
    dst
}

/// Set `n` bytes at `dst` to the low byte of `value`.
///
/// # Safety
///
/// `dst` must be writable for `n` bytes.
pub unsafe fn fill(dst: *mut u8, value: i32, n: usize) -> *mut u8 {
    let byte = value as u8;
    let word = u32::from_ne_bytes([byte; WORD]);
    let (mut d, mut n) = (dst, n);
    unsafe {
        while n > 0 && misalignment(d) != 0 {
            *d = byte;
            (d, n) = (d.add(1), n - 1);
        }
        while n >= WORD {
            *(d as *mut u32) = word;
            (d, n) = (d.add(WORD), n - WORD);
        }
        while n > 0 {
            *d = byte;
            (d, n) = (d.add(1), n - 1);
        }
    }
    dst
}

/// Compare `n` bytes as unsigned: negative, zero or positive as `a` is
/// below, equal to or above `b`.
///
/// # Safety
///
/// `a` and `b` must be readable for `n` bytes.
pub unsafe fn compare(a: *const u8, b: *const u8, n: usize) -> i32 {
    for i in 0..n {
        let (x, y) = unsafe { (*a.add(i), *b.add(i)) };
        if x != y {
            return x as i32 - y as i32;
        }
    }
    0
}

#[cfg(all(target_arch = "x86", not(test)))]
mod exports {
    use core::ffi::{c_int, c_void};

    #[unsafe(no_mangle)]
    unsafe extern "C" fn memcpy(dst: *mut c_void, src: *const c_void, n: usize) -> *mut c_void {
        unsafe { super::copy(dst.cast(), src.cast(), n).cast() }
    }

    #[unsafe(no_mangle)]
    unsafe extern "C" fn memmove(dst: *mut c_void, src: *const c_void, n: usize) -> *mut c_void {
        unsafe { super::copy_overlapping(dst.cast(), src.cast(), n).cast() }
    }

    #[unsafe(no_mangle)]
    unsafe extern "C" fn memset(dst: *mut c_void, value: c_int, n: usize) -> *mut c_void {
        unsafe { super::fill(dst.cast(), value, n).cast() }
    }

    #[unsafe(no_mangle)]
    unsafe extern "C" fn memcmp(a: *const c_void, b: *const c_void, n: usize) -> c_int {
        unsafe { super::compare(a.cast(), b.cast(), n) }
    }

    /// memcmp when only equality matters; LLVM emits it for slice
    /// equality.
    #[unsafe(no_mangle)]
    unsafe extern "C" fn bcmp(a: *const c_void, b: *const c_void, n: usize) -> c_int {
        unsafe { super::compare(a.cast(), b.cast(), n) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Every offset in a word and every length up to a few words, so each
    /// loop runs with every alignment.
    const OFFSETS: core::ops::Range<usize> = 0..2 * WORD;
    const LENGTHS: core::ops::Range<usize> = 0..5 * WORD;
    const SIZE: usize = 8 * WORD;

    #[repr(align(4))]
    #[derive(Clone, Copy, Debug, PartialEq)]
    struct Buffer([u8; SIZE]);

    fn counting() -> Buffer {
        Buffer(core::array::from_fn(|i| i as u8 + 1))
    }

    #[test]
    fn copy_matches_a_byte_copy() {
        for dst in OFFSETS {
            // A source past the destination, so the regions never meet.
            for src in OFFSETS.map(|o| o + 4 * WORD) {
                for n in 0..2 * WORD + 1 {
                    let mut buf = counting();
                    let mut expected = buf;
                    expected.0.copy_within(src..src + n, dst);
                    let base = buf.0.as_mut_ptr();
                    let out = unsafe { copy(base.add(dst), base.add(src), n) };
                    assert_eq!(out, base.wrapping_add(dst));
                    assert_eq!(buf, expected, "dst {dst} src {src} n {n}");
                }
            }
        }
    }

    #[test]
    fn copy_overlapping_handles_both_directions() {
        for dst in OFFSETS {
            for src in OFFSETS {
                for n in LENGTHS {
                    let mut buf = counting();
                    let mut expected = buf;
                    expected.0.copy_within(src..src + n, dst);
                    let base = buf.0.as_mut_ptr();
                    let out = unsafe { copy_overlapping(base.add(dst), base.add(src), n) };
                    assert_eq!(out, base.wrapping_add(dst));
                    assert_eq!(buf, expected, "dst {dst} src {src} n {n}");
                }
            }
        }
    }

    #[test]
    fn fill_stores_the_low_byte() {
        for dst in OFFSETS {
            for n in LENGTHS {
                let mut buf = Buffer([0; SIZE]);
                let mut expected = buf;
                expected.0[dst..dst + n].fill(0xAB);
                let base = buf.0.as_mut_ptr();
                let out = unsafe { fill(base.add(dst), 0x1AB, n) };
                assert_eq!(out, base.wrapping_add(dst));
                assert_eq!(buf, expected, "dst {dst} n {n}");
            }
        }
    }

    #[test]
    fn compare_orders_bytes_unsigned() {
        let cmp = |a: &[u8], b: &[u8]| unsafe { compare(a.as_ptr(), b.as_ptr(), a.len()) };
        assert_eq!(cmp(b"abc", b"abc"), 0);
        assert!(cmp(b"abc", b"abd") < 0);
        assert!(cmp(b"abd", b"abc") > 0);
        assert!(cmp(&[0x80], &[0x01]) > 0);
        assert_eq!(cmp(b"", b""), 0);
        // Only the first n bytes count.
        assert_eq!(unsafe { compare(b"ab".as_ptr(), b"ac".as_ptr(), 1) }, 0);
        let (a, mut b) = (counting(), counting());
        b.0[SIZE - 1] += 1;
        assert!(cmp(&a.0, &b.0) < 0);
        assert!(cmp(&b.0, &a.0) > 0);
    }
}
