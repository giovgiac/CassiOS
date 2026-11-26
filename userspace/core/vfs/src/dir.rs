// dir.rs -- FAT32 directory entries and names
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Entries are read and written as 32-byte slices of a sector. Long names
// keep only the low byte of each UTF-16 unit (the system is ASCII only).

pub const ENTRY_SIZE: usize = 32;

pub const ATTR_READ_ONLY: u8 = 0x01;
pub const ATTR_HIDDEN: u8 = 0x02;
pub const ATTR_SYSTEM: u8 = 0x04;
pub const ATTR_VOLUME_ID: u8 = 0x08;
pub const ATTR_DIRECTORY: u8 = 0x10;
pub const ATTR_ARCHIVE: u8 = 0x20;
pub const ATTR_LFN: u8 = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID;

/// First name byte of an unused entry, and of the end of the directory.
pub const FREE: u8 = 0xE5;
pub const END: u8 = 0x00;

const LFN_LAST: u8 = 0x40;
/// Characters per long-name entry, at these byte offsets.
pub const LFN_CHARS: usize = 13;
const LFN_OFFSETS: [usize; LFN_CHARS] = [1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30];

/// Printable characters a short (8.3) name may not hold.
const SHORT_INVALID: &[u8] = b"*?<>|\":+,;=[]\\";

/// Whether a short name may not hold `c` (control characters included);
/// '_' replaces such characters.
fn short_invalid(c: u8) -> bool {
    c < 0x20 || c == 0x7F || SHORT_INVALID.contains(&c)
}

/// Whether `name` may be stored as a long name: FAT forbids control
/// characters and these printable ones even there.
pub fn valid_long_name(name: &[u8]) -> bool {
    !name.iter().any(|&c| c < 0x20 || b"\\:*?\"<>|".contains(&c))
}

/// Highest "~N" number `numbered_short_name` takes.
pub const MAX_SHORT_NUMBER: u32 = 999_999;

/// Longest name a path component may have.
pub const NAME_MAX: usize = 255;

/// The fields of a short (8.3) directory entry this driver uses.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Entry {
    pub name: [u8; 11],
    pub attr: u8,
    /// Case flags for names that fit 8.3 (used by mtools instead of LFN).
    pub case: u8,
    pub cluster: u32,
    pub size: u32,
}

impl Entry {
    pub fn parse(raw: &[u8]) -> Self {
        let u16_at = |o: usize| u16::from_le_bytes([raw[o], raw[o + 1]]) as u32;
        Self {
            name: raw[..11].try_into().unwrap(),
            attr: raw[11],
            case: raw[12],
            cluster: u16_at(20) << 16 | u16_at(26),
            size: u32::from_le_bytes(raw[28..32].try_into().unwrap()),
        }
    }

    /// Write this entry over `raw`, clearing the fields it does not use.
    pub fn store(&self, raw: &mut [u8]) {
        raw[..ENTRY_SIZE].fill(0);
        raw[..11].copy_from_slice(&self.name);
        raw[11] = self.attr;
        raw[12] = self.case;
        self.store_location(raw);
    }

    /// Update only the cluster and size fields of `raw`.
    pub fn store_location(&self, raw: &mut [u8]) {
        raw[20..22].copy_from_slice(&((self.cluster >> 16) as u16).to_le_bytes());
        raw[26..28].copy_from_slice(&(self.cluster as u16).to_le_bytes());
        raw[28..32].copy_from_slice(&self.size.to_le_bytes());
    }

    pub fn is_dir(&self) -> bool {
        self.attr & ATTR_DIRECTORY != 0
    }

    /// The "." and ".." entries every subdirectory starts with.
    pub fn is_dot(&self) -> bool {
        self.name[0] == b'.' && (self.name[1] == b' ' || self.name[1] == b'.')
    }
}

/// What a raw entry is, for walking a directory.
pub enum Kind {
    End,
    Free,
    LongName,
    /// Volume labels and the dot entries, which listings skip.
    Hidden,
    Visible(Entry),
}

pub fn classify(raw: &[u8]) -> Kind {
    match raw[0] {
        END => return Kind::End,
        FREE => return Kind::Free,
        _ => {}
    }
    // The top two attribute bits are reserved.
    if raw[11] & 0x3F == ATTR_LFN {
        return Kind::LongName;
    }
    let entry = Entry::parse(raw);
    if entry.attr & ATTR_VOLUME_ID != 0 || entry.is_dot() {
        Kind::Hidden
    } else {
        Kind::Visible(entry)
    }
}

/// Long-name entries a name of NAME_MAX characters needs.
const MAX_LONG_ENTRIES: usize = NAME_MAX.div_ceil(LFN_CHARS);

/// Collects long-name entries that precede a short entry, in disk order.
pub struct LongName {
    chars: [u8; MAX_LONG_ENTRIES * LFN_CHARS],
    entries: usize,
}

impl LongName {
    pub const fn new() -> Self {
        Self {
            chars: [0; MAX_LONG_ENTRIES * LFN_CHARS],
            entries: 0,
        }
    }

    pub fn clear(&mut self) {
        self.entries = 0;
    }

    pub fn is_empty(&self) -> bool {
        self.entries == 0
    }

    /// Record the next long-name entry. Disk order is last part first, so
    /// entry k from the end holds characters 13*k.. of the name.
    pub fn push(&mut self, raw: &[u8]) {
        if self.entries == MAX_LONG_ENTRIES {
            return;
        }
        // Store each entry's characters in push order; `read_into` walks
        // them back to front. Only the low byte of each unit is kept, so
        // 0x0000 and 0xFFFF (end and padding) become 0x00 and 0xFF.
        let base = self.entries * LFN_CHARS;
        for (i, &o) in LFN_OFFSETS.iter().enumerate() {
            self.chars[base + i] = raw[o];
        }
        self.entries += 1;
    }

    /// Write the name into `out` and return its length. Entries were pushed
    /// last part first, so read them back from the last pushed to the first,
    /// stopping at the first 0x0000 or 0xFFFF unit.
    pub fn read_into(&self, out: &mut [u8; NAME_MAX]) -> usize {
        let mut len = 0;
        for e in (0..self.entries).rev() {
            for i in 0..LFN_CHARS {
                let c = self.chars[e * LFN_CHARS + i];
                if c == 0 || c == 0xFF || len == NAME_MAX {
                    return len;
                }
                out[len] = c;
                len += 1;
            }
        }
        len
    }
}

/// A short entry's name as text: base, then "." and extension if any, with
/// the case flags applied.
pub fn short_name_text(entry: &Entry, out: &mut [u8; NAME_MAX]) -> usize {
    let lower = |c: u8, flag: u8| {
        if entry.case & flag != 0 {
            c.to_ascii_lowercase()
        } else {
            c
        }
    };
    let mut len = 0;
    for &c in entry.name[..8].iter().take_while(|&&c| c != b' ') {
        out[len] = lower(c, 0x08);
        len += 1;
    }
    if entry.name[8] != b' ' {
        out[len] = b'.';
        len += 1;
        for &c in entry.name[8..].iter().take_while(|&&c| c != b' ') {
            out[len] = lower(c, 0x10);
            len += 1;
        }
    }
    len
}

/// The 8.3 name first tried for `name`: uppercased, truncated, characters
/// that 8.3 forbids replaced by '_', and marked with "~N" when a long name
/// is stored too and the base is long, has an extension or lost characters
/// (N from the name length, as the C++ driver did). `create` numbers it
/// further when another entry holds it. A plain upper-case 8.3 name is
/// stored as is: it has no long name, so a "~N" would make it unreachable
/// by its own name.
pub fn short_name(name: &[u8]) -> [u8; 11] {
    let base_len = name.iter().rposition(|&c| c == b'.').unwrap_or(name.len());
    // Spaces and dots are dropped from the base.
    let lossy = name.iter().any(|&c| short_invalid(c))
        || name[..base_len].iter().any(|&c| c == b' ' || c == b'.');
    let tail = (base_len > 8 || base_len < name.len() || lossy) && needs_long_name(name);
    build_short_name(name, tail.then_some(name.len() as u32 % 9 + 1))
}

/// The short name for `name` with "~n" as its tail, for when the usual one
/// is taken (1 <= n <= MAX_SHORT_NUMBER). The name then needs long entries.
pub fn numbered_short_name(name: &[u8], n: u32) -> [u8; 11] {
    build_short_name(name, Some(n))
}

fn build_short_name(name: &[u8], tail: Option<u32>) -> [u8; 11] {
    let mut short = [b' '; 11];
    let last_dot = name.iter().rposition(|&c| c == b'.');
    let base_len = last_dot.unwrap_or(name.len());

    let short_char = |c: u8| {
        if short_invalid(c) {
            b'_'
        } else {
            c.to_ascii_uppercase()
        }
    };
    let mut j = 0;
    for &c in name[..base_len].iter().filter(|&&c| c != b' ' && c != b'.') {
        if j == 8 {
            break;
        }
        short[j] = short_char(c);
        j += 1;
    }
    if let Some(n) = tail {
        let mut digits = [0u8; 7];
        let mut count = 0;
        let mut v = n;
        loop {
            digits[count] = b'0' + (v % 10) as u8;
            count += 1;
            v /= 10;
            if v == 0 {
                break;
            }
        }
        // "~" and the digits replace the end of the base if needed.
        let at = j.min(7 - count);
        short[at] = b'~';
        for k in 0..count {
            short[at + 1 + k] = digits[count - 1 - k];
        }
    }
    if let Some(dot) = last_dot {
        for (k, &c) in name[dot + 1..].iter().take(3).enumerate() {
            short[8 + k] = short_char(c);
        }
    }
    short
}

/// Whether `name` needs long-name entries (anything that is not a plain
/// upper-case 8.3 name with a non-empty base, and an extension if it has a
/// dot).
pub fn needs_long_name(name: &[u8]) -> bool {
    if name.len() > 12 {
        return true;
    }
    let mut dot = None;
    for (i, &c) in name.iter().enumerate() {
        match c {
            b'.' if dot.is_some() => return true,
            b'.' => dot = Some(i),
            b'a'..=b'z' | b' ' => return true,
            _ if short_invalid(c) => return true,
            _ => {}
        }
    }
    let (base, ext_fits) = match dot {
        Some(d) => (d, (1..=3).contains(&(name.len() - d - 1))),
        None => (name.len(), true),
    };
    base == 0 || base > 8 || !ext_fits
}

/// Checksum of a short name, stored in each of its long-name entries.
pub fn checksum(short: &[u8; 11]) -> u8 {
    short
        .iter()
        .fold(0u8, |sum, &c| sum.rotate_right(1).wrapping_add(c))
}

/// Number of long-name entries `name` needs.
pub fn long_entries(name: &[u8]) -> usize {
    name.len().div_ceil(LFN_CHARS)
}

/// Fill `raw` as long-name entry `seq` (1-based, `last` for the final one)
/// for `name`. Characters past the end are 0x0000 then 0xFFFF padding.
pub fn store_long_entry(raw: &mut [u8], name: &[u8], seq: usize, last: bool, sum: u8) {
    raw[..ENTRY_SIZE].fill(0xFF);
    raw[0] = seq as u8 | if last { LFN_LAST } else { 0 };
    raw[11] = ATTR_LFN;
    raw[12] = 0;
    raw[13] = sum;
    raw[26] = 0;
    raw[27] = 0;
    for (i, &o) in LFN_OFFSETS.iter().enumerate() {
        let pos = (seq - 1) * LFN_CHARS + i;
        let unit: u16 = match pos.cmp(&name.len()) {
            core::cmp::Ordering::Less => name[pos] as u16,
            core::cmp::Ordering::Equal => 0,
            core::cmp::Ordering::Greater => 0xFFFF,
        };
        raw[o..o + 2].copy_from_slice(&unit.to_le_bytes());
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn text(entry: &Entry) -> alloc::vec::Vec<u8> {
        let mut out = [0; NAME_MAX];
        let len = short_name_text(entry, &mut out);
        out[..len].to_vec()
    }

    fn long_name_of(name: &[u8]) -> alloc::vec::Vec<u8> {
        let n = long_entries(name);
        let mut lfn = LongName::new();
        let mut raw = [0u8; ENTRY_SIZE];
        // Disk order: highest sequence first.
        for seq in (1..=n).rev() {
            store_long_entry(&mut raw, name, seq, seq == n, 0);
            lfn.push(&raw);
        }
        let mut out = [0; NAME_MAX];
        let len = lfn.read_into(&mut out);
        out[..len].to_vec()
    }

    #[test]
    fn entry_round_trip() {
        let e = Entry {
            name: *b"README  TXT",
            attr: ATTR_ARCHIVE,
            case: 0,
            cluster: 0x0012_3456,
            size: 1234,
        };
        let mut raw = [0xAA; ENTRY_SIZE];
        e.store(&mut raw);
        assert_eq!(Entry::parse(&raw), e);
        assert_eq!(&raw[20..22], &[0x12, 0x00]);
        assert_eq!(&raw[26..28], &[0x56, 0x34]);
    }

    #[test]
    fn short_names_as_text() {
        let mut e = Entry {
            name: *b"README  TXT",
            ..Entry::default()
        };
        assert_eq!(text(&e), b"README.TXT");
        e.case = 0x08 | 0x10;
        assert_eq!(text(&e), b"readme.txt");
        e.case = 0x10;
        assert_eq!(text(&e), b"README.txt");
        e.name = *b"BIN        ";
        assert_eq!(text(&e), b"BIN");
    }

    #[test]
    fn short_name_generation_matches_the_c_driver() {
        assert_eq!(&short_name(b"readme.txt"), b"README~2TXT");
        assert_eq!(&short_name(b"note"), b"NOTE       ");
        assert_eq!(&short_name(b"DIR"), b"DIR        ");
        assert_eq!(&short_name(b"averylongname"), b"AVERYL~5   ");
        assert_eq!(&short_name(b"a b.c"), b"AB~6    C  ");
    }

    #[test]
    fn invalid_short_name_characters_are_replaced() {
        assert_eq!(&short_name(b"A+B"), b"A_B~4      ");
        assert_eq!(&short_name(b"X*Y.T;T"), b"X_Y~8   T_T");
        assert_eq!(&short_name(b"A\tB"), b"A_B~4      ");
    }

    #[test]
    fn names_that_lose_characters_get_a_tail() {
        assert_eq!(&short_name(b" "), b"~2         ");
        assert_eq!(&short_name(b"a b"), b"AB~4       ");
        assert_eq!(&short_name(b"MY FILE"), b"MYFILE~8   ");
    }

    #[test]
    fn forbidden_long_name_characters() {
        assert!(valid_long_name(b"a+b,c;d=e[f].txt"));
        for name in [
            &b"a\\b"[..],
            b"a:b",
            b"a*b",
            b"a?b",
            b"a\"b",
            b"a<b",
            b"a>b",
            b"a|b",
            b"a\tb",
        ] {
            assert!(!valid_long_name(name));
        }
    }

    #[test]
    fn numbered_short_names() {
        assert_eq!(&numbered_short_name(b"NOTE", 1), b"NOTE~1     ");
        assert_eq!(
            &numbered_short_name(b"averylongname.txt", 12),
            b"AVERY~12TXT"
        );
        assert_eq!(&numbered_short_name(b"x", MAX_SHORT_NUMBER), b"X~999999   ");
        assert_eq!(
            &numbered_short_name(b"abcdefgh", MAX_SHORT_NUMBER),
            b"A~999999   "
        );
    }

    #[test]
    fn plain_upper_case_names_keep_their_short_name() {
        // The C++ driver stored README~2TXT with no long name here, so the
        // file read back as README~2.TXT.
        assert_eq!(&short_name(b"README.TXT"), b"README  TXT");
        assert_eq!(&short_name(b"NOTE.TXT"), b"NOTE    TXT");
    }

    #[test]
    fn which_names_need_long_entries() {
        assert!(!needs_long_name(b"README.TXT"));
        assert!(!needs_long_name(b"BIN"));
        assert!(needs_long_name(b"readme.txt"));
        assert!(needs_long_name(b"A.B.C"));
        assert!(needs_long_name(b"TOOLONGNAME"));
        assert!(needs_long_name(b"FILE.TEXT"));
        assert!(needs_long_name(b"MY FILE"));
        assert!(needs_long_name(b"NOTE."));
        assert!(needs_long_name(b".TXT"));
        assert!(needs_long_name(b"A+B"));
        assert!(needs_long_name(b"A.B[C"));
        assert!(needs_long_name(b"A\tB"));
        assert!(needs_long_name(b"A\x7fB"));
    }

    #[test]
    fn checksum_matches_the_spec_algorithm() {
        let mut sum = 0u8;
        for &c in b"README  TXT" {
            sum = (if sum & 1 != 0 { 0x80u8 } else { 0 })
                .wrapping_add(sum >> 1)
                .wrapping_add(c);
        }
        assert_eq!(checksum(b"README  TXT"), sum);
    }

    #[test]
    fn long_names_round_trip() {
        for name in [
            &b"a"[..],
            b"exactly13char",
            b"fourteen chars",
            b"a-much-longer-name-spanning-several-entries.txt",
        ] {
            assert_eq!(long_name_of(name), name);
        }
        let max = [b'x'; NAME_MAX];
        assert_eq!(long_name_of(&max), &max[..]);
    }

    #[test]
    fn long_entry_layout() {
        let mut raw = [0u8; ENTRY_SIZE];
        store_long_entry(&mut raw, b"ab", 1, true, 0x5A);
        assert_eq!(raw[0], 0x41);
        assert_eq!((raw[11], raw[13]), (ATTR_LFN, 0x5A));
        assert_eq!(&raw[1..7], &[b'a', 0, b'b', 0, 0, 0]);
        assert_eq!(&raw[7..9], &[0xFF, 0xFF]);
        assert_eq!(&raw[26..28], &[0, 0]);
    }

    #[test]
    fn classify_entries() {
        let mut raw = [0u8; ENTRY_SIZE];
        assert!(matches!(classify(&raw), Kind::End));
        raw[0] = FREE;
        assert!(matches!(classify(&raw), Kind::Free));
        raw[..11].copy_from_slice(b".          ");
        raw[11] = ATTR_DIRECTORY;
        assert!(matches!(classify(&raw), Kind::Hidden));
        raw[..11].copy_from_slice(b"LABEL      ");
        raw[11] = ATTR_VOLUME_ID;
        assert!(matches!(classify(&raw), Kind::Hidden));
        raw[11] = ATTR_LFN;
        assert!(matches!(classify(&raw), Kind::LongName));
        raw[11] = ATTR_LFN | 0xC0;
        assert!(matches!(classify(&raw), Kind::LongName));
        raw[11] = ATTR_ARCHIVE;
        assert!(matches!(classify(&raw), Kind::Visible(_)));
    }
}
