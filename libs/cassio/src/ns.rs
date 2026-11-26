// ns.rs -- nameserver protocol
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use crate::ipc::Message;

/// Well-known PID of the nameserver (always the first userspace process).
pub const PID: u32 = 1;

/// Maximum length of a service name, excluding the NUL terminator.
pub const MAX_NAME_LEN: usize = 16;

/// Entry returned by NsListAll: a NUL-padded name and its PID.
#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct Entry {
    pub name: [u8; 20],
    pub pid: u32,
}

/// Pack up to MAX_NAME_LEN bytes of `name` into arg1..arg4, little-endian.
/// Packing stops at the first NUL.
pub fn pack_name(name: &[u8], msg: &mut Message) {
    let mut words = [0u32; 4];
    for (i, &b) in name
        .iter()
        .take(MAX_NAME_LEN)
        .take_while(|&&b| b != 0)
        .enumerate()
    {
        words[i / 4] |= (b as u32) << ((i % 4) * 8);
    }
    [msg.arg1, msg.arg2, msg.arg3, msg.arg4] = words;
}

/// Unpack a name from arg1..arg4 into `out` and return it up to the
/// first NUL.
pub fn unpack_name<'a>(msg: &Message, out: &'a mut [u8; MAX_NAME_LEN + 1]) -> &'a [u8] {
    let words = [msg.arg1, msg.arg2, msg.arg3, msg.arg4];
    for i in 0..MAX_NAME_LEN {
        out[i] = (words[i / 4] >> ((i % 4) * 8)) as u8;
    }
    out[MAX_NAME_LEN] = 0;
    let len = out.iter().position(|&b| b == 0).unwrap();
    &out[..len]
}

/// Register `name` for the calling process. Returns 1 on success, 0 if
/// the name is already registered or the nameserver is unreachable.
#[cfg(target_arch = "x86")]
pub fn register_name(name: &[u8]) -> u32 {
    request(crate::ipc::message_type::NS_REGISTER, name)
}

/// Look up the PID registered for `name`. Returns 0 if it is not
/// registered or the nameserver is unreachable.
#[cfg(target_arch = "x86")]
pub fn lookup(name: &[u8]) -> u32 {
    request(crate::ipc::message_type::NS_LOOKUP, name)
}

/// The PID of service `name`, retrying the lookup a bounded number of
/// times (500 tries, sleeping 10 ms between them) while it starts. Logs and
/// returns 0 if it never appears, so requests to it fail instead of hang.
#[cfg(target_arch = "x86")]
pub fn connect(name: &[u8]) -> u32 {
    const TRIES: u32 = 500;
    const WAIT_MS: u32 = 10;
    for _ in 0..TRIES {
        let pid = lookup(name);
        if pid != 0 {
            return pid;
        }
        crate::os::sleep(WAIT_MS);
    }
    crate::os::write(2, name);
    crate::os::write(2, b": service not found\n");
    0
}

/// Fill `out` with the registered names; returns how many it holds.
#[cfg(target_arch = "x86")]
pub fn list_all(out: &mut [Entry]) -> usize {
    let mut msg = Message {
        kind: crate::ipc::message_type::NS_LIST_ALL,
        ..Message::default()
    };
    // SAFETY: Entry is plain repr(C) data, valid for any bytes.
    let bytes = unsafe {
        core::slice::from_raw_parts_mut(out.as_mut_ptr() as *mut u8, core::mem::size_of_val(out))
    };
    if crate::ipc::send_into(PID, &mut msg, bytes) < 0 {
        return 0;
    }
    (msg.arg1 as usize).min(out.len())
}

impl Entry {
    /// The name, up to its first NUL.
    pub fn name(&self) -> &[u8] {
        let len = self
            .name
            .iter()
            .position(|&b| b == 0)
            .unwrap_or(self.name.len());
        &self.name[..len]
    }
}

#[cfg(target_arch = "x86")]
fn request(kind: u32, name: &[u8]) -> u32 {
    let mut msg = Message {
        kind,
        ..Message::default()
    };
    pack_name(name, &mut msg);
    if crate::ipc::send(PID, &mut msg, &[]) < 0 {
        return 0;
    }
    msg.arg1
}

#[cfg(test)]
mod tests {
    use super::*;

    fn roundtrip(name: &[u8]) -> ([u8; MAX_NAME_LEN + 1], usize) {
        let mut msg = Message::default();
        pack_name(name, &mut msg);
        let mut out = [0xFF; MAX_NAME_LEN + 1];
        let len = unpack_name(&msg, &mut out).len();
        (out, len)
    }

    #[test]
    fn pack_layout_is_little_endian() {
        let mut msg = Message::default();
        pack_name(b"abcde", &mut msg);
        assert_eq!(msg.arg1, u32::from_le_bytes(*b"abcd"));
        assert_eq!(msg.arg2, b'e' as u32);
        assert_eq!((msg.arg3, msg.arg4), (0, 0));
    }

    #[test]
    fn roundtrip_short_name() {
        let (out, len) = roundtrip(b"hello");
        assert_eq!(&out[..len], b"hello");
        assert_eq!(out[5], 0);
    }

    #[test]
    fn roundtrip_max_length() {
        let (out, len) = roundtrip(b"0123456789abcdef");
        assert_eq!(&out[..len], b"0123456789abcdef");
        assert_eq!(out[16], 0);
    }

    #[test]
    fn long_name_truncated() {
        let (out, len) = roundtrip(b"0123456789abcdefXYZ");
        assert_eq!(&out[..len], b"0123456789abcdef");
    }

    #[test]
    fn empty_name() {
        let (out, len) = roundtrip(b"");
        assert_eq!(len, 0);
        assert_eq!(out[0], 0);
    }

    #[test]
    fn stops_at_nul() {
        let (out, len) = roundtrip(b"ab\0cd");
        assert_eq!(&out[..len], b"ab");
    }

    #[test]
    fn entry_name_stops_at_nul() {
        let mut e = Entry::default();
        e.name[..3].copy_from_slice(b"kbd");
        assert_eq!(e.name(), b"kbd");
        e.name = [b'x'; 20];
        assert_eq!(e.name().len(), 20);
    }

    #[test]
    fn entry_layout_matches_cpp() {
        assert_eq!(core::mem::size_of::<Entry>(), 24);
        assert_eq!(core::mem::size_of::<Message>(), 24);
    }
}
