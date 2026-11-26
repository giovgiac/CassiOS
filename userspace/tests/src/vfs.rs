// vfs.rs -- VFS service tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// They run against the test disk: an empty FAT32 volume with the files
// under disk/ copied in (README.TXT and bin/hello.elf).

use alloc::vec::Vec;

use cassio::vfs::{Stat, Vfs};

use crate::harness::Outcome;

tests!(
    vfs_mkdir_and_list,
    vfs_open_write_read,
    vfs_remove_file,
    vfs_remove_directory,
    vfs_long_path,
    vfs_large_write_read,
    vfs_remove_root_fails,
    vfs_duplicate_name_fails,
    vfs_remove_nonempty_dir_fails,
    vfs_read_with_offset,
    vfs_empty_file_in_subdir,
    vfs_open_nonexistent_fails,
    vfs_dotdot_traversal,
    vfs_dot_traversal,
    vfs_seed_file_readable,
    vfs_stat,
    vfs_close_releases_handles,
);

/// Every name in the directory at `path`.
fn names(vfs: &Vfs, path: &[u8]) -> Vec<Vec<u8>> {
    let mut out = Vec::new();
    while let Some(name) = vfs.list(path, out.len() as u32) {
        out.push(name);
    }
    out
}

/// The whole file at `path`, or None if it cannot be opened.
fn read_file(vfs: &Vfs, path: &[u8]) -> Option<Vec<u8>> {
    let handle = vfs.open(path, false)?;
    let mut buf = alloc::vec![0u8; 256];
    let n = vfs.read(handle, 0, &mut buf);
    vfs.close(handle);
    buf.truncate(n);
    Some(buf)
}

fn vfs_mkdir_and_list() -> Outcome {
    let vfs = Vfs::connect();
    check!(vfs.mkdir(b"/testdir"));
    check!(names(&vfs, b"/").iter().any(|n| n == b"testdir"));
    Ok(())
}

fn vfs_open_write_read() -> Outcome {
    let vfs = Vfs::connect();
    let Some(handle) = vfs.open(b"/hello", true) else {
        return Err("open failed".into());
    };
    check!(vfs.write(handle, b"hello"));
    let mut buf = [0u8; 256];
    check_eq!(vfs.read(handle, 0, &mut buf), 5);
    check_eq!(&buf[..5], b"hello");
    check!(vfs.close(handle));
    Ok(())
}

fn vfs_remove_file() -> Outcome {
    let vfs = Vfs::connect();
    if let Some(handle) = vfs.open(b"/delme", true) {
        vfs.close(handle);
    }
    check!(vfs.remove(b"/delme"));
    check!(!names(&vfs, b"/").iter().any(|n| n == b"delme"));
    Ok(())
}

fn vfs_remove_directory() -> Outcome {
    let vfs = Vfs::connect();
    vfs.mkdir(b"/rmdir");
    check!(vfs.remove(b"/rmdir"));
    check!(!names(&vfs, b"/").iter().any(|n| n == b"rmdir"));
    Ok(())
}

fn vfs_long_path() -> Outcome {
    let vfs = Vfs::connect();
    check!(vfs.mkdir(b"/longdir"));
    check!(vfs.mkdir(b"/longdir/subdir"));
    let handle = vfs.open(b"/longdir/subdir/file", true);
    check!(handle.is_some());
    handle.map(|h| vfs.close(h));
    check_eq!(names(&vfs, b"/longdir/subdir"), [b"file".to_vec()]);
    Ok(())
}

fn vfs_large_write_read() -> Outcome {
    let vfs = Vfs::connect();
    let Some(handle) = vfs.open(b"/bigfile", true) else {
        return Err("open failed".into());
    };
    // Several sectors' worth, in one request.
    let data: Vec<u8> = (0..3000).map(|i| (i % 251) as u8).collect();
    check!(vfs.write(handle, &data));
    let mut buf = alloc::vec![0u8; 4096];
    check_eq!(vfs.read(handle, 0, &mut buf), data.len());
    check!(buf[..data.len()] == data[..]);
    vfs.close(handle);
    Ok(())
}

fn vfs_remove_root_fails() -> Outcome {
    check!(!Vfs::connect().remove(b"/"));
    Ok(())
}

fn vfs_duplicate_name_fails() -> Outcome {
    let vfs = Vfs::connect();
    check!(vfs.mkdir(b"/dupdir"));
    check!(!vfs.mkdir(b"/dupdir"));
    Ok(())
}

fn vfs_remove_nonempty_dir_fails() -> Outcome {
    let vfs = Vfs::connect();
    vfs.mkdir(b"/nonempty");
    if let Some(handle) = vfs.open(b"/nonempty/child", true) {
        vfs.close(handle);
    }
    check!(!vfs.remove(b"/nonempty"));
    Ok(())
}

fn vfs_read_with_offset() -> Outcome {
    let vfs = Vfs::connect();
    let Some(handle) = vfs.open(b"/offtest", true) else {
        return Err("open failed".into());
    };
    vfs.write(handle, b"abcd");
    let mut buf = [0u8; 256];
    check_eq!(vfs.read(handle, 2, &mut buf), 2);
    check_eq!(&buf[..2], b"cd");
    check_eq!(vfs.read(handle, 4, &mut buf), 0);
    vfs.close(handle);
    Ok(())
}

fn vfs_empty_file_in_subdir() -> Outcome {
    let vfs = Vfs::connect();
    vfs.mkdir(b"/subtest");
    let h1 = vfs.open(b"/subtest/file.txt", true);
    check!(h1.is_some());
    // Opening the same empty file without create must find it.
    let Some(h2) = vfs.open(b"/subtest/file.txt", false) else {
        return Err("reopen failed".into());
    };
    check!(vfs.write(h2, b"OK"));
    let mut buf = [0u8; 32];
    check_eq!(vfs.read(h2, 0, &mut buf), 2);
    check_eq!(&buf[..2], b"OK");
    // One entry, not two.
    check_eq!(names(&vfs, b"/subtest"), [b"file.txt".to_vec()]);
    h1.map(|h| vfs.close(h));
    vfs.close(h2);
    Ok(())
}

fn vfs_open_nonexistent_fails() -> Outcome {
    check!(Vfs::connect().open(b"/doesnotexist", false).is_none());
    Ok(())
}

fn vfs_dotdot_traversal() -> Outcome {
    let vfs = Vfs::connect();
    vfs.mkdir(b"/dottest");
    let text = read_file(&vfs, b"/dottest/../dottest/../README.TXT");
    check!(text.is_some_and(|t| t.starts_with(b"Welcome")));
    Ok(())
}

fn vfs_dot_traversal() -> Outcome {
    let text = read_file(&Vfs::connect(), b"/./README.TXT");
    check!(text.is_some_and(|t| t.starts_with(b"Welcome")));
    Ok(())
}

fn vfs_seed_file_readable() -> Outcome {
    let text = read_file(&Vfs::connect(), b"/README.TXT");
    check!(text.is_some_and(|t| t.starts_with(b"Welcome")));
    Ok(())
}

fn vfs_stat() -> Outcome {
    let vfs = Vfs::connect();
    check_eq!(vfs.stat(b"/README.TXT"), Stat::File);
    check_eq!(vfs.stat(b"/bin"), Stat::Directory);
    check_eq!(vfs.stat(b"/missing"), Stat::Missing);
    Ok(())
}

fn vfs_close_releases_handles() -> Outcome {
    let vfs = Vfs::connect();
    // More opens than the service has handles: each close frees one.
    for _ in 0..40 {
        let Some(handle) = vfs.open(b"/README.TXT", false) else {
            return Err("open failed".into());
        };
        check!(vfs.close(handle));
        check!(!vfs.close(handle));
    }
    check!(!vfs.close(0));
    Ok(())
}
