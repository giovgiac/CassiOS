// elf.rs -- ELF executable loader
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Loads 32-bit i386 executables into a user address space: every PT_LOAD
// segment is copied into fresh frames, the rest of its memory zeroed. The
// program's heap starts at the first page after its segments. The C++
// ElfLoader (kernel/include/core/elf.hpp) forwards here until its callers
// are ported.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

use crate::memory::KERNEL_VBASE;

const PAGE: u32 = 4096;
const HEADER_SIZE: usize = 52;
const PROGRAM_HEADER_SIZE: usize = 32;
const PT_LOAD: u32 = 1;

/// A segment to load: `filesz` bytes from `offset` in the file to
/// `vaddr`, then zeros up to `memsz`.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Segment {
    pub offset: u32,
    pub vaddr: u32,
    pub filesz: u32,
    pub memsz: u32,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Program {
    pub entry: u32,
    /// Page-aligned end of the highest segment.
    pub heap_start: u32,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Error {
    /// Not a 32-bit little-endian i386 executable.
    NotAnExecutable,
    /// A header or segment lies outside the file.
    Truncated,
    /// A segment is larger in the file than in memory, or would reach
    /// kernel space.
    BadSegment,
    /// No frame left to load into.
    OutOfMemory,
}

fn u16_at(data: &[u8], at: usize) -> u16 {
    u16::from_le_bytes([data[at], data[at + 1]])
}

fn u32_at(data: &[u8], at: usize) -> u32 {
    u32::from_le_bytes(data[at..at + 4].try_into().unwrap())
}

/// Every program header of the executable in `data`: Some(segment) for
/// PT_LOAD, None for other types, Err if the header lies outside the file.
/// Expects a valid ELF header.
fn program_headers(data: &[u8]) -> impl Iterator<Item = Result<Option<Segment>, Error>> + '_ {
    let phoff = u32_at(data, 28) as usize;
    let phnum = u16_at(data, 44) as usize;
    (0..phnum).map(move |i| {
        let at = phoff
            .checked_add(i * PROGRAM_HEADER_SIZE)
            .filter(|&at| at.saturating_add(PROGRAM_HEADER_SIZE) <= data.len())
            .ok_or(Error::Truncated)?;
        if u32_at(data, at) != PT_LOAD {
            return Ok(None);
        }
        Ok(Some(Segment {
            offset: u32_at(data, at + 4),
            vaddr: u32_at(data, at + 8),
            filesz: u32_at(data, at + 16),
            memsz: u32_at(data, at + 20),
        }))
    })
}

/// Validate the executable in `data`. Allocates nothing: a program file
/// is user input, and the kernel heap may be short.
pub fn parse(data: &[u8]) -> Result<Program, Error> {
    if data.len() < HEADER_SIZE {
        return Err(Error::Truncated);
    }
    let ident_ok = data[..4] == *b"\x7FELF" && data[4] == 1 && data[5] == 1;
    // e_type ET_EXEC, e_machine EM_386, 32-byte program headers.
    let header_ok = u16_at(data, 16) == 2
        && u16_at(data, 18) == 3
        && u16_at(data, 42) as usize == PROGRAM_HEADER_SIZE;
    if !ident_ok || !header_ok {
        return Err(Error::NotAnExecutable);
    }

    let mut end = 0u64;
    for header in program_headers(data) {
        let Some(segment) = header? else {
            continue;
        };
        if segment.offset as u64 + segment.filesz as u64 > data.len() as u64 {
            return Err(Error::Truncated);
        }
        let segment_end = segment.vaddr as u64 + segment.memsz as u64;
        if segment.filesz > segment.memsz || segment_end > KERNEL_VBASE as u64 {
            return Err(Error::BadSegment);
        }
        end = end.max(segment_end);
    }
    Ok(Program {
        entry: u32_at(data, 24),
        heap_start: end.next_multiple_of(PAGE as u64) as u32,
    })
}

/// The PT_LOAD segments of an executable `parse` accepted.
pub fn segments(data: &[u8]) -> impl Iterator<Item = Segment> + '_ {
    program_headers(data).filter_map(|header| header.ok().flatten())
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use super::*;
    use crate::memory::paging::{
        PAGE_PRESENT, PAGE_READWRITE, PAGE_USER, map_user_page, translate, zeroed_frame,
    };
    use crate::memory::phys_to_virt;

    /// Load the executable in `data` into the address space whose
    /// directory is at `directory`. On error, pages already mapped stay
    /// in it: the caller destroys the address space.
    pub fn load(directory: u32, data: &[u8]) -> Result<Program, Error> {
        let program = parse(data)?;
        let flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
        for segment in segments(data) {
            let file = &data[segment.offset as usize..][..segment.filesz as usize];
            let start = segment.vaddr & !(PAGE - 1);
            let end = segment.vaddr as u64 + segment.memsz as u64;
            for page in (start as u64..end).step_by(PAGE as usize) {
                let page = page as u32;
                // A page an earlier segment already mapped keeps its frame.
                let frame = match translate(directory, page) {
                    Some(frame) => frame,
                    None => {
                        let frame = zeroed_frame().ok_or(Error::OutOfMemory)?;
                        map_user_page(directory, page, frame, flags);
                        frame
                    }
                };
                // The part of the segment in this page: file bytes, then
                // zeros (a fresh frame is zeroed, a shared one may not be).
                let from = page.max(segment.vaddr);
                let to = (page as u64 + PAGE as u64).min(end) as u32;
                // SAFETY: the frame is mapped above and reached through the
                // direct map; [from, to) lies within its page.
                let bytes = unsafe {
                    core::slice::from_raw_parts_mut(
                        phys_to_virt(frame + from % PAGE),
                        (to - from) as usize,
                    )
                };
                // The file bytes for this part, if any are left.
                let rest = file.get((from - segment.vaddr) as usize..).unwrap_or(&[]);
                let copied = rest.len().min(bytes.len());
                bytes[..copied].copy_from_slice(&rest[..copied]);
                bytes[copied..].fill(0);
            }
        }
        Ok(program)
    }

    /// Load for C++: true on success, with the entry point and the initial
    /// heap break stored through `entry` and `heap_start`.
    ///
    /// # Safety
    ///
    /// `data` must point to `size` readable bytes, and `entry` and
    /// `heap_start` must be writable.
    #[unsafe(no_mangle)]
    unsafe extern "C" fn elf_load(
        directory: u32,
        data: *const u8,
        size: usize,
        entry: *mut u32,
        heap_start: *mut u32,
    ) -> bool {
        let data = unsafe { core::slice::from_raw_parts(data, size) };
        match load(directory, data) {
            Ok(program) => {
                unsafe {
                    *entry = program.entry;
                    *heap_start = program.heap_start;
                }
                true
            }
            Err(_) => false,
        }
    }

    #[cfg(not(test))]
    pub mod ktests {
        use alloc::vec::Vec;

        use super::*;
        use crate::ktest::Outcome;
        use crate::memory::paging::{create_address_space, destroy_address_space};
        use crate::memory::physical::PHYSICAL;

        tests!(
            elf_load_places_every_byte,
            elf_load_keeps_a_page_two_segments_share,
            elf_load_failure_reports_an_error,
        );

        /// The byte at `virt` in the address space at `directory`.
        fn peek(directory: u32, virt: u32) -> Option<u8> {
            let frame = translate(directory, virt)?;
            Some(unsafe { *phys_to_virt(frame + virt % PAGE) })
        }

        fn elf_load_places_every_byte() -> Outcome {
            // Unaligned, with zeros past the file data.
            let image = tests::executable(&[(0x0040_0010, &[1, 2, 3, 4], 0x2000)]);
            let directory = create_address_space().unwrap_or(0);
            check!(directory != 0);
            let loaded = load(directory, &image);
            let bytes: Vec<_> = [0x0040_0010, 0x0040_0013, 0x0040_0014, 0x0040_2000 + 0xF]
                .iter()
                .map(|&v| peek(directory, v))
                .collect();
            let beyond = peek(directory, 0x0040_3000);
            destroy_address_space(directory);
            check!(loaded.is_ok());
            check_eq!(loaded.unwrap().heap_start, 0x0040_3000);
            check_eq!(bytes, [Some(1), Some(4), Some(0), Some(0)]);
            check_eq!(beyond, None);
            Ok(())
        }

        fn elf_load_keeps_a_page_two_segments_share() -> Outcome {
            let image = tests::executable(&[
                (0x0040_0000, &[0xAA; 16], 16),
                (0x0040_0800, &[0xBB; 16], 16),
            ]);
            let before = PHYSICAL.with(|p| p.free_frames());
            let directory = create_address_space().unwrap_or(0);
            check!(directory != 0);
            let loaded = load(directory, &image);
            let first = peek(directory, 0x0040_0000);
            let second = peek(directory, 0x0040_0800);
            destroy_address_space(directory);
            check!(loaded.is_ok());
            check_eq!((first, second), (Some(0xAA), Some(0xBB)));
            // One frame for the shared page, all of it given back.
            check_eq!(PHYSICAL.with(|p| p.free_frames()), before);
            Ok(())
        }

        fn elf_load_failure_reports_an_error() -> Outcome {
            let directory = create_address_space().unwrap_or(0);
            check!(directory != 0);
            let result = load(directory, b"not an executable at all, not even close......");
            destroy_address_space(directory);
            check_eq!(result, Err(Error::Truncated));
            Ok(())
        }
    }
}

#[cfg(any(test, target_arch = "x86"))]
pub mod tests {
    use alloc::vec::Vec;

    use super::*;

    /// An executable with one PT_LOAD per (vaddr, file bytes, memsz), the
    /// data after the headers, entry at the first segment.
    pub fn executable(segments: &[(u32, &[u8], u32)]) -> Vec<u8> {
        let mut image = alloc::vec![0u8; HEADER_SIZE + segments.len() * PROGRAM_HEADER_SIZE];
        image[..6].copy_from_slice(b"\x7FELF\x01\x01");
        image[16..18].copy_from_slice(&2u16.to_le_bytes());
        image[18..20].copy_from_slice(&3u16.to_le_bytes());
        image[24..28].copy_from_slice(&segments.first().map_or(0, |s| s.0).to_le_bytes());
        image[28..32].copy_from_slice(&(HEADER_SIZE as u32).to_le_bytes());
        image[42..44].copy_from_slice(&(PROGRAM_HEADER_SIZE as u16).to_le_bytes());
        image[44..46].copy_from_slice(&(segments.len() as u16).to_le_bytes());
        for (i, (vaddr, bytes, memsz)) in segments.iter().enumerate() {
            let offset = image.len() as u32;
            image.extend_from_slice(bytes);
            let at = HEADER_SIZE + i * PROGRAM_HEADER_SIZE;
            let fields = [PT_LOAD, offset, *vaddr, *vaddr, bytes.len() as u32, *memsz];
            for (j, field) in fields.iter().enumerate() {
                image[at + j * 4..at + j * 4 + 4].copy_from_slice(&field.to_le_bytes());
            }
        }
        image
    }

    #[cfg(test)]
    fn set_u32(image: &mut [u8], at: usize, value: u32) {
        image[at..at + 4].copy_from_slice(&value.to_le_bytes());
    }

    #[cfg(test)]
    const SEGMENT: usize = HEADER_SIZE;

    #[test]
    fn parse_lists_the_load_segments() {
        let image = executable(&[(0x0040_0000, &[0xDE, 0xAD, 0xBE, 0xEF], 4)]);
        let program = parse(&image).unwrap();
        assert_eq!(program.entry, 0x0040_0000);
        assert_eq!(program.heap_start, 0x0040_1000);
        assert_eq!(
            segments(&image).collect::<Vec<_>>(),
            [Segment {
                offset: (HEADER_SIZE + PROGRAM_HEADER_SIZE) as u32,
                vaddr: 0x0040_0000,
                filesz: 4,
                memsz: 4
            }]
        );
    }

    #[test]
    fn heap_starts_after_the_highest_segment() {
        let image = executable(&[(0x0050_0000, &[1], 0x1001), (0x0040_0000, &[2], 1)]);
        assert_eq!(parse(&image).unwrap().heap_start, 0x0050_2000);
    }

    #[test]
    fn other_segment_types_are_skipped() {
        let mut image = executable(&[(0x0040_0000, &[1], 1)]);
        set_u32(&mut image, SEGMENT, 0x6474_E551); // PT_GNU_STACK
        assert!(parse(&image).is_ok());
        assert_eq!(segments(&image).count(), 0);
    }

    #[test]
    fn non_executables_are_rejected() {
        let good = executable(&[(0x0040_0000, &[1], 1)]);
        // Magic, class, byte order, type, machine, program header size.
        for (at, value) in [(0, 0x7Eu8), (4, 2), (5, 2), (16, 3), (18, 0x28), (42, 0)] {
            let mut image = good.clone();
            image[at] = value;
            assert_eq!(parse(&image), Err(Error::NotAnExecutable), "byte {at}");
        }
    }

    #[test]
    fn truncated_files_are_rejected() {
        let image = executable(&[(0x0040_0000, &[1, 2, 3, 4], 4)]);
        assert_eq!(parse(&image[..HEADER_SIZE - 1]), Err(Error::Truncated));
        // The program header runs past the end.
        assert_eq!(parse(&image[..HEADER_SIZE + 8]), Err(Error::Truncated));
        // The segment's data runs past the end.
        assert_eq!(parse(&image[..image.len() - 1]), Err(Error::Truncated));
    }

    #[test]
    fn segment_offsets_that_wrap_are_rejected() {
        let mut image = executable(&[(0x0040_0000, &[1, 2, 3, 4], 4)]);
        set_u32(&mut image, SEGMENT + 4, 0xFFFF_FFF0);
        assert_eq!(parse(&image), Err(Error::Truncated));
        let past_the_end = image.len() as u32 + 1;
        set_u32(&mut image, SEGMENT + 4, past_the_end);
        set_u32(&mut image, SEGMENT + 16, 0);
        assert_eq!(parse(&image), Err(Error::Truncated));
    }

    #[test]
    fn segments_larger_in_the_file_are_rejected() {
        let image = executable(&[(0x0040_0000, &[1, 2, 3, 4], 2)]);
        assert_eq!(parse(&image), Err(Error::BadSegment));
    }

    #[test]
    fn segments_reaching_kernel_space_are_rejected() {
        let kernel = executable(&[(KERNEL_VBASE, &[1], 1)]);
        assert_eq!(parse(&kernel), Err(Error::BadSegment));
        let straddling = executable(&[(KERNEL_VBASE - 0x1000, &[1], 0x1001)]);
        assert_eq!(parse(&straddling), Err(Error::BadSegment));
        let wrapping = executable(&[(0xFFFF_F000, &[1], 0x2000)]);
        assert_eq!(parse(&wrapping), Err(Error::BadSegment));
        let last_user_page = executable(&[(KERNEL_VBASE - 0x1000, &[1], 0x1000)]);
        assert!(parse(&last_user_page).is_ok());
    }
}
