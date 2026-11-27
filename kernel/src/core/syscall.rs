// syscall.rs -- system calls and IPC
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// `int 0x80` enters through syscallEntry (syscall_stub.s), an interrupt
// gate, so syscalls run with interrupts disabled; only sleep and the
// serial write enable them while they wait, outside the process table's
// borrow. The number is in eax and the arguments in ebx, ecx, edx, esi
// and edi; the result goes back in eax. A blocking call (send, receive,
// waitpid) returns another process's frame, and its own result is set
// when it is woken.
//
// IPC is synchronous send/receive/reply plus fire-and-forget notify.
// receive takes, in order: a pending IRQ, a queued notification, then the
// first blocked sender. Messages and data are copied between address
// spaces by switching CR3 around each copy.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

use crate::memory::KERNEL_VBASE;

/// The lowest page of the user stack; the heap must stay below it.
pub const USER_STACK_BOTTOM: u32 = KERNEL_VBASE - 4096;

/// The break after growing the heap at `old` by `increment` bytes,
/// rounded up to a page; None if it wraps or reaches the user stack.
pub fn grown_break(old: u32, increment: u32) -> Option<u32> {
    let end = old as u64 + increment as u64;
    let rounded = end.next_multiple_of(4096);
    (rounded <= USER_STACK_BOTTOM as u64).then_some(rounded as u32)
}

/// Whether [ptr, ptr + len) lies below kernel space without wrapping.
pub fn below_kernel(ptr: u32, len: u32) -> bool {
    ptr as u64 + len as u64 <= KERNEL_VBASE as u64
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;
    use core::mem::size_of;

    use sys::io::{Port, PortType};
    use sys::ipc::Message;
    use sys::os::{FramebufferInfo, ProcEntry, syscall};

    use super::*;
    use crate::hardware::interrupt::{enable, set_gate};
    use crate::hardware::{irq, pit, serial::COM1};
    use crate::memory::paging::{
        PAGE_DEVICE, PAGE_PRESENT, PAGE_READWRITE, PAGE_USER, create_address_space,
        destroy_address_space, flush_tlb, map_user_page, range_unmapped, translate,
        user_range_mapped,
    };
    use crate::memory::physical::{FRAME_SIZE, PHYSICAL};
    use crate::sync::Global;
    use crate::system::elf;
    use crate::system::process::{Frame, KERNEL_PID, PROCESSES, State, Table, set_return};
    use crate::system::scheduler::{exit_current, reschedule};

    static FRAMEBUFFER: Global<FramebufferInfo> = Global::new(FramebufferInfo {
        address: 0,
        width: 0,
        height: 0,
        pitch: 0,
        bpp: 0,
    });

    unsafe extern "C" {
        fn syscallEntry();
    }

    /// Install the syscall gate (DPL 3, so user mode can raise it).
    pub fn install() {
        set_gate(0x80, syscallEntry, 3);
    }

    pub fn set_framebuffer(info: FramebufferInfo) {
        FRAMEBUFFER.with(|fb| *fb = info);
    }

    fn cr3() -> u32 {
        let value: u32;
        unsafe { asm!("mov {}, cr3", out(reg) value, options(nomem, nostack)) };
        value
    }

    /// Whether [ptr, ptr + len) is user memory of the caller: below kernel
    /// space, not wrapping, every page mapped and user-accessible.
    fn in_user_space(ptr: u32, len: u32) -> bool {
        below_kernel(ptr, len) && user_range_mapped(cr3(), ptr, len)
    }

    /// Whether every buffer a syscall names is the caller's user memory,
    /// so a program can neither make the kernel touch kernel memory for it
    /// nor fault it on an unmapped address. Callers in ring 0 (the kernel
    /// tests) pass kernel buffers and are trusted.
    fn buffers_valid(f: &Frame) -> bool {
        if !f.from_user() {
            return true;
        }
        let message = size_of::<Message>() as u32;
        match f.eax {
            syscall::SEND | syscall::REPLY | syscall::NOTIFY => {
                in_user_space(f.ecx, message) && in_user_space(f.esi, f.edi)
            }
            syscall::RECEIVE => in_user_space(f.ebx, message) && in_user_space(f.esi, f.edi),
            syscall::WRITE => in_user_space(f.ecx, f.edx),
            syscall::PROC_LIST => f
                .ecx
                .checked_mul(size_of::<ProcEntry>() as u32)
                .is_some_and(|len| in_user_space(f.ebx, len)),
            syscall::EXEC => in_user_space(f.ebx, f.ecx),
            _ => true,
        }
    }

    /// Run `copy` with `directory` loaded (0 or the current one: as is).
    fn in_address_space<R>(directory: u32, copy: impl FnOnce() -> R) -> R {
        let current = cr3();
        if directory == 0 || directory == current {
            return copy();
        }
        unsafe { asm!("mov cr3, {}", in(reg) directory, options(nostack)) };
        let result = copy();
        unsafe { asm!("mov cr3, {}", in(reg) current, options(nostack)) };
        result
    }

    /// Copy `len` bytes from `src` in the current address space to `dst`
    /// in `directory`'s, through a kernel buffer.
    fn copy_to(directory: u32, dst: u32, src: u32, len: u32) {
        let mut chunk = [0u8; 256];
        let mut done = 0;
        while done < len {
            let n = (len - done).min(chunk.len() as u32) as usize;
            unsafe {
                core::ptr::copy_nonoverlapping((src + done) as *const u8, chunk.as_mut_ptr(), n);
                in_address_space(directory, || {
                    core::ptr::copy_nonoverlapping(chunk.as_ptr(), (dst + done) as *mut u8, n)
                });
            }
            done += n as u32;
        }
    }

    /// Copy `len` bytes from `src` in `directory`'s address space to `dst`
    /// in the current one, through a kernel buffer.
    fn copy_from(directory: u32, dst: u32, src: u32, len: u32) {
        let mut chunk = [0u8; 256];
        let mut done = 0;
        while done < len {
            let n = (len - done).min(chunk.len() as u32) as usize;
            unsafe {
                in_address_space(directory, || {
                    core::ptr::copy_nonoverlapping((src + done) as *const u8, chunk.as_mut_ptr(), n)
                });
                core::ptr::copy_nonoverlapping(chunk.as_ptr(), (dst + done) as *mut u8, n);
            }
            done += n as u32;
        }
    }

    fn read_message(ptr: u32) -> Message {
        unsafe { (ptr as *const Message).read_unaligned() }
    }

    /// Write `msg` to the user buffer at `ptr` in `directory`'s space.
    pub fn write_message(directory: u32, ptr: u32, msg: &Message) {
        in_address_space(directory, || unsafe {
            (ptr as *mut Message).write_unaligned(*msg)
        });
    }

    /// Deliver `msg` (and up to `len` bytes of data at `data` in the
    /// current address space) to `target`, blocked in receive, waking it
    /// with `result`.
    fn deliver(table: &mut Table, target: u32, msg: &Message, data: u32, len: u32, result: u32) {
        let process = table.get_mut(target).unwrap();
        write_message(process.directory, process.msg_ptr, msg);
        if data != 0 && len > 0 && process.data_ptr != 0 && process.data_len > 0 {
            copy_to(
                process.directory,
                process.data_ptr,
                data,
                len.min(process.data_len),
            );
        }
        set_return(process, result);
        process.state = State::Ready;
    }

    /// Send the message at `msg` to `target` and block until it replies
    /// (into the same buffer). 0 to block, -1 on error.
    pub fn send(table: &mut Table, target: u32, msg: u32, data: u32, len: u32) -> i32 {
        let sender = table.current_pid();
        if target == sender || table.get(target).is_none() {
            return -1;
        }
        let message = read_message(msg);
        {
            let process = table.current();
            process.msg = message;
            process.msg_ptr = msg;
            process.data_ptr = data;
            process.data_len = len;
            // So it is woken with an error if the target ends first.
            process.wait_pid = target;
        }
        if table.get(target).unwrap().state == State::ReceiveBlocked {
            deliver(table, target, &message, data, len, sender);
        } else if !table.get_mut(target).unwrap().push_sender(sender) {
            return -1;
        }
        table.current().state = State::SendBlocked;
        0
    }

    /// What receive found.
    #[derive(Debug, PartialEq, Eq)]
    pub enum Received {
        /// A message from the kernel (an IRQ) or a notification: no reply.
        Notification,
        /// A message from a blocked sender, who waits for a reply.
        Send(u32),
        /// Nothing yet: block.
        Block,
        /// The queued sender is gone.
        Error,
    }

    /// Receive into the buffer at `msg` (data into `data`, up to
    /// `capacity` bytes).
    pub fn receive(table: &mut Table, msg: u32, data: u32, capacity: u32) -> Received {
        let receiver = table.current_pid();
        {
            let process = table.current();
            process.data_ptr = data;
            process.data_len = capacity;
        }
        if let Some(irq) = irq::take_pending(receiver) {
            let notify = Message {
                kind: sys::ipc::message_type::IRQ_NOTIFY,
                arg1: irq as u32,
                ..Message::default()
            };
            unsafe { (msg as *mut Message).write_unaligned(notify) };
            return Received::Notification;
        }
        if let Some(n) = table.current().pop_notification() {
            unsafe { (msg as *mut Message).write_unaligned(n.msg) };
            if data != 0 && capacity > 0 && !n.data.is_empty() {
                let len = n.data.len().min(capacity as usize);
                unsafe {
                    core::ptr::copy_nonoverlapping(n.data.as_ptr(), data as *mut u8, len);
                }
            }
            return Received::Notification;
        }
        if let Some(sender) = table.current().pop_sender() {
            let Some(process) = table.get(sender) else {
                return Received::Error;
            };
            let (message, directory) = (process.msg, process.directory);
            let (sender_data, sender_len) = (process.data_ptr, process.data_len);
            unsafe { (msg as *mut Message).write_unaligned(message) };
            if sender_data != 0 && sender_len > 0 && data != 0 && capacity > 0 {
                copy_from(directory, data, sender_data, sender_len.min(capacity));
            }
            return Received::Send(sender);
        }
        let process = table.current();
        process.msg_ptr = msg;
        process.state = State::ReceiveBlocked;
        Received::Block
    }

    /// Reply with the message at `msg` to `target`, blocked sending to the
    /// caller. 0, or -1 if it is not waiting.
    pub fn reply(table: &mut Table, target: u32, msg: u32, data: u32, len: u32) -> i32 {
        let Some(process) = table.get(target) else {
            return -1;
        };
        if process.state != State::SendBlocked {
            return -1;
        }
        // Read the reply here: the sender's directory is loaded to write.
        let message = read_message(msg);
        deliver(table, target, &message, data, len, 0);
        0
    }

    /// Queue the message at `msg` for `target` without blocking. 0, or -1
    /// if there is no such process or memory is exhausted.
    pub fn notify(table: &mut Table, target: u32, msg: u32, data: u32, len: u32) -> i32 {
        if target == table.current_pid() || table.get(target).is_none() {
            return -1;
        }
        let message = read_message(msg);
        if table.get(target).unwrap().state == State::ReceiveBlocked {
            // 0 as the sender: no reply expected.
            deliver(table, target, &message, data, len, 0);
            return 0;
        }
        // Keep a copy: the sender's buffer may be gone by delivery.
        let bytes: &[u8] = if data != 0 && len > 0 {
            unsafe { core::slice::from_raw_parts(data as *const u8, len as usize) }
        } else {
            &[]
        };
        let process = table.get_mut(target).unwrap();
        if process.push_notification(message, bytes) {
            0
        } else {
            -1
        }
    }

    /// Start the program in the file at `file` (in the caller's address
    /// space). Its pid, or 0.
    fn exec(table: &mut Table, file: u32, size: u32) -> u32 {
        if file == 0 || size == 0 {
            return 0;
        }
        // The file stays readable: the caller's directory is loaded while
        // the loader writes the new one through the direct map.
        let data = unsafe { core::slice::from_raw_parts(file as *const u8, size as usize) };
        let Some(directory) = create_address_space() else {
            return 0;
        };
        let Ok(program) = elf::load(directory, data) else {
            destroy_address_space(directory);
            return 0;
        };
        let (cs, ds) = (table.current().cs, table.current().ds);
        match table.spawn(directory, program.entry, program.heap_start, cs, ds) {
            Some(pid) => pid,
            None => {
                destroy_address_space(directory);
                0
            }
        }
    }

    /// Grow the caller's heap by `increment` bytes. The old break, or 0.
    fn sbrk(table: &mut Table, increment: u32) -> u32 {
        let process = table.current();
        let old = process.heap_break;
        if old == 0 {
            return 0;
        }
        let Some(new) = grown_break(old, increment) else {
            return 0;
        };
        let flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
        for page in (old.next_multiple_of(FRAME_SIZE)..new).step_by(FRAME_SIZE as usize) {
            // A page mapped by an earlier, failed sbrk is reused.
            if translate(process.directory, page).is_some() {
                continue;
            }
            let Some(frame) = PHYSICAL.with(|p| p.alloc()) else {
                return 0;
            };
            map_user_page(process.directory, page, frame, flags);
            flush_tlb(page);
        }
        process.heap_break = new;
        old
    }

    fn proc_list(table: &Table, buf: u32, max: u32) -> u32 {
        let entries = table
            .iter()
            .filter(|p| p.pid != KERNEL_PID)
            .take(max as usize);
        let mut count = 0;
        for p in entries {
            let entry = ProcEntry {
                pid: p.pid,
                state: p.state as u32,
                heap_size: p.heap_size(),
            };
            unsafe { (buf as *mut ProcEntry).add(count).write_unaligned(entry) };
            count += 1;
        }
        count as u32
    }

    /// Map `pages` pages of device memory at `physical` to `virt` in the
    /// caller. 0, or -1 (kernel task, bad range, pages already in use).
    fn map_device(table: &mut Table, physical: u32, virt: u32, pages: u32) -> i32 {
        let directory = table.current().directory;
        if directory == 0 || pages == 0 || pages > 0x10000 {
            return -1;
        }
        let size = pages * FRAME_SIZE;
        if physical.checked_add(size).is_none() || !below_kernel(virt, size) {
            return -1;
        }
        // Mapping over pages in use would drop their frames unfreed
        // (device pages are never freed on exit).
        if !range_unmapped(directory, virt, size) {
            return -1;
        }
        // The frames are the device's, so exiting must not free them.
        let flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER | PAGE_DEVICE;
        for i in 0..pages {
            map_user_page(
                directory,
                virt + i * FRAME_SIZE,
                physical + i * FRAME_SIZE,
                flags,
            );
        }
        0
    }

    fn waitpid(table: &mut Table, pid: u32) -> i32 {
        let caller = table.current_pid();
        if pid == caller || table.get(pid).is_none() {
            return -1;
        }
        let process = table.current();
        process.wait_pid = pid;
        process.state = State::WaitBlocked;
        0
    }

    /// fd 2 is the serial port. Polling the UART takes about 0.26 ms per
    /// character, so interrupts stay enabled meanwhile.
    fn write(fd: u32, buf: u32, len: u32) -> i32 {
        if fd != 2 {
            return -1;
        }
        let bytes = unsafe { core::slice::from_raw_parts(buf as *const u8, len as usize) };
        enable();
        COM1.write(bytes);
        unsafe { asm!("cli", options(nomem, nostack)) };
        len as i32
    }

    /// Waits for timer ticks, so interrupts stay enabled meanwhile and the
    /// caller may be switched out.
    fn sleep(ms: u32) -> i32 {
        enable();
        pit::sleep(ms);
        unsafe { asm!("cli", options(nomem, nostack)) };
        0
    }

    /// Handle the syscall whose frame is at `esp`. Returns the frame to
    /// resume.
    pub fn handle(esp: u32) -> u32 {
        // SAFETY: the stub passes the frame it just saved.
        let f = unsafe { &mut *(esp as *mut Frame) };
        if !buffers_valid(f) {
            // ProcList and Exec report failure as 0 (no entries, no process).
            let zero = f.eax == syscall::PROC_LIST || f.eax == syscall::EXEC;
            f.eax = if zero { 0 } else { u32::MAX };
            return esp;
        }
        // These two enable interrupts, so they run outside the table's
        // borrow: the timer and IRQs reach it meanwhile.
        match f.eax {
            syscall::WRITE => {
                f.eax = write(f.ebx, f.ecx, f.edx) as u32;
                return esp;
            }
            syscall::SLEEP => {
                f.eax = sleep(f.ebx) as u32;
                return esp;
            }
            _ => {}
        }
        PROCESSES.with(|table| dispatch(table, f, esp))
    }

    fn dispatch(table: &mut Table, f: &mut Frame, esp: u32) -> u32 {
        let blocked = |table: &mut Table, f: &mut Frame, result: i32| {
            if result == 0 {
                reschedule(table, esp)
            } else {
                f.eax = result as u32;
                esp
            }
        };
        match f.eax {
            syscall::SEND => {
                let result = send(table, f.ebx, f.ecx, f.esi, f.edi);
                blocked(table, f, result)
            }
            syscall::RECEIVE => match receive(table, f.ebx, f.esi, f.edi) {
                Received::Notification => {
                    f.eax = 0;
                    esp
                }
                Received::Send(pid) => {
                    f.eax = pid;
                    esp
                }
                Received::Error => {
                    f.eax = u32::MAX;
                    esp
                }
                Received::Block => reschedule(table, esp),
            },
            syscall::REPLY => {
                f.eax = reply(table, f.ebx, f.ecx, f.esi, f.edi) as u32;
                esp
            }
            syscall::NOTIFY => {
                f.eax = notify(table, f.ebx, f.ecx, f.esi, f.edi) as u32;
                esp
            }
            syscall::IRQ_REGISTER => {
                f.eax = irq::register_forward(f.ebx, table.current_pid()) as u32;
                esp
            }
            syscall::UPTIME => {
                f.eax = pit::ticks();
                esp
            }
            syscall::REBOOT => {
                // The keyboard controller pulses the reset line.
                unsafe { Port::<u8>::new(PortType::KbdCommand).write(0xFE) };
                esp
            }
            syscall::SHUTDOWN => {
                unsafe { asm!("cli", "hlt", options(nomem, nostack)) };
                esp
            }
            syscall::EXIT => {
                // Ends a QEMU test run (0 passes, anything else fails); does
                // nothing without the debug-exit device.
                let code = if f.ebx == 0 { 0x00 } else { 0x01 };
                unsafe { Port::<u8>::new(PortType::QemuDebugExit).write(code) };
                exit_current(table, esp)
            }
            syscall::MAP_DEVICE => {
                f.eax = map_device(table, f.ebx, f.ecx, f.edx) as u32;
                esp
            }
            syscall::MEM_INFO => {
                let (total, used, free) =
                    PHYSICAL.with(|p| (p.total_frames(), p.used_frames(), p.free_frames()));
                f.eax = total;
                f.ebx = used;
                f.ecx = free;
                esp
            }
            syscall::SBRK => {
                f.eax = sbrk(table, f.ebx);
                esp
            }
            syscall::PROC_LIST => {
                f.eax = proc_list(table, f.ebx, f.ecx);
                esp
            }
            syscall::EXEC => {
                f.eax = exec(table, f.ebx, f.ecx);
                esp
            }
            syscall::WAIT_PID => {
                let result = waitpid(table, f.ebx);
                blocked(table, f, result)
            }
            syscall::FRAMEBUFFER_INFO => {
                let fb = FRAMEBUFFER.with(|fb| *fb);
                f.eax = fb.address;
                f.ebx = fb.width;
                f.ecx = fb.height;
                f.edx = fb.pitch;
                f.esi = fb.bpp;
                esp
            }
            _ => {
                f.eax = u32::MAX;
                esp
            }
        }
    }

    /// Called by syscallEntry with the saved frame.
    #[unsafe(no_mangle)]
    extern "C" fn handleSyscall(esp: u32) -> u32 {
        handle(esp)
    }

    /// For C++ start(): the multiboot framebuffer.
    #[unsafe(no_mangle)]
    extern "C" fn syscall_set_framebuffer(
        address: u32,
        width: u32,
        height: u32,
        pitch: u32,
        bpp: u32,
    ) {
        set_framebuffer(FramebufferInfo {
            address,
            width,
            height,
            pitch,
            bpp,
        });
    }

    #[cfg(not(test))]
    pub mod ktests {
        use alloc::vec::Vec;

        use super::*;
        use crate::ktest::Outcome;
        use crate::system::process::testing::{add, restore, snapshot, with_table};

        tests!(
            ipc_send_to_receive_blocked_delivers_immediately,
            ipc_send_to_non_blocked_queues_sender,
            ipc_receive_with_pending_sender,
            ipc_receive_empty_queue_blocks,
            ipc_reply_unblocks_sender,
            ipc_reply_to_non_blocked_fails,
            ipc_send_to_self_fails,
            ipc_send_to_invalid_pid_fails,
            ipc_full_round_trip,
            ipc_send_receive_blocked_with_data,
            ipc_send_data_truncated,
            ipc_send_no_data,
            ipc_reply_with_data,
            ipc_full_round_trip_with_data,
            ipc_receive_queued_sender_with_data,
            ipc_notify_queue_with_data,
            ipc_notify_queue_data_truncated,
            ipc_notify_to_receive_blocked_delivers_with_sender_0,
            sbrk_returns_old_break,
            sbrk_zero_returns_current_break,
            sbrk_maps_multiple_pages,
            sbrk_no_heap_returns_zero,
            sbrk_overflow_returns_zero,
            sbrk_stack_collision_returns_zero,
            proclist_skips_kernel_task,
            proclist_includes_created_process,
            proclist_zero_heap_size_when_no_heap,
            proclist_respects_max_entries,
            syscall_uptime_is_monotonic,
            syscall_write_to_other_fds_fails,
            syscall_invalid_number_fails,
            irq_forward_delivers_to_receive_blocked_process,
            irq_forward_sets_pending_when_not_receive_blocked,
            irq_forward_coexists_with_the_pit,
            irq_forward_irq_takes_priority_over_send_queue,
        );

        fn message(kind: u32, arg1: u32) -> Message {
            Message {
                kind,
                arg1,
                ..Message::default()
            }
        }

        fn ptr<T>(value: &mut T) -> u32 {
            value as *mut T as u32
        }

        /// Make `pid` the caller of the IPC function under test.
        fn as_caller(table: &mut Table, pid: u32) {
            table.set_current(pid);
        }

        /// A process blocked in receive into `buf` (data into `data`).
        fn receiving(
            table: &mut Table,
            frame: &mut Frame,
            buf: &mut Message,
            data: &mut [u8],
        ) -> u32 {
            let pid = add(table, frame, State::ReceiveBlocked);
            let p = table.get_mut(pid).unwrap();
            p.msg_ptr = ptr(buf);
            p.data_ptr = if data.is_empty() {
                0
            } else {
                data.as_mut_ptr() as u32
            };
            p.data_len = data.len() as u32;
            pid
        }

        fn ipc_send_to_receive_blocked_delivers_immediately() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let mut inbox = Message::default();
                let receiver = receiving(table, &mut rf, &mut inbox, &mut []);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(42, 7);
                check_eq!(send(table, receiver, ptr(&mut out), 0, 0), 0);
                check_eq!(inbox, out);
                check_eq!(rf.eax, sender);
                check_eq!(table.get(receiver).unwrap().state, State::Ready);
                check_eq!(table.get(sender).unwrap().state, State::SendBlocked);
                Ok(())
            })
        }

        fn ipc_send_to_non_blocked_queues_sender() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let receiver = add(table, &mut rf, State::Ready);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(1, 2);
                check_eq!(send(table, receiver, ptr(&mut out), 0, 0), 0);
                check_eq!(table.get(receiver).unwrap().first_sender(), Some(sender));
                check_eq!(table.get(sender).unwrap().state, State::SendBlocked);
                Ok(())
            })
        }

        fn ipc_receive_with_pending_sender() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let receiver = add(table, &mut rf, State::Ready);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(5, 6);
                send(table, receiver, ptr(&mut out), 0, 0);
                as_caller(table, receiver);
                let mut inbox = Message::default();
                check_eq!(
                    receive(table, ptr(&mut inbox), 0, 0),
                    Received::Send(sender)
                );
                check_eq!(inbox, out);
                Ok(())
            })
        }

        fn ipc_receive_empty_queue_blocks() -> Outcome {
            with_table(|table| {
                let mut rf = Frame::default();
                let receiver = add(table, &mut rf, State::Running);
                as_caller(table, receiver);
                let mut inbox = Message::default();
                check_eq!(receive(table, ptr(&mut inbox), 0, 0), Received::Block);
                let p = table.get(receiver).unwrap();
                check_eq!(p.state, State::ReceiveBlocked);
                check_eq!(p.msg_ptr, ptr(&mut inbox));
                Ok(())
            })
        }

        fn ipc_reply_unblocks_sender() -> Outcome {
            with_table(|table| {
                let mut sf = Frame::default();
                let mut reply_buf = Message::default();
                let sender = add(table, &mut sf, State::SendBlocked);
                table.get_mut(sender).unwrap().msg_ptr = ptr(&mut reply_buf);
                let mut answer = message(9, 10);
                check_eq!(reply(table, sender, ptr(&mut answer), 0, 0), 0);
                check_eq!(reply_buf, answer);
                check_eq!(sf.eax, 0);
                check_eq!(table.get(sender).unwrap().state, State::Ready);
                Ok(())
            })
        }

        fn ipc_reply_to_non_blocked_fails() -> Outcome {
            with_table(|table| {
                let mut f = Frame::default();
                let pid = add(table, &mut f, State::Ready);
                let mut answer = message(1, 1);
                check_eq!(reply(table, pid, ptr(&mut answer), 0, 0), -1);
                Ok(())
            })
        }

        fn ipc_send_to_self_fails() -> Outcome {
            with_table(|table| {
                let mut f = Frame::default();
                let pid = add(table, &mut f, State::Running);
                as_caller(table, pid);
                let mut out = message(1, 1);
                check_eq!(send(table, pid, ptr(&mut out), 0, 0), -1);
                check_eq!(notify(table, pid, ptr(&mut out), 0, 0), -1);
                Ok(())
            })
        }

        fn ipc_send_to_invalid_pid_fails() -> Outcome {
            with_table(|table| {
                let mut out = message(1, 1);
                check_eq!(send(table, 9999, ptr(&mut out), 0, 0), -1);
                check_eq!(notify(table, 9999, ptr(&mut out), 0, 0), -1);
                Ok(())
            })
        }

        fn ipc_full_round_trip() -> Outcome {
            round_trip(&[], &mut [], &[], &mut [])
        }

        fn ipc_full_round_trip_with_data() -> Outcome {
            round_trip(b"request", &mut [0; 16], b"response", &mut [0; 16])
        }

        /// Server receives (blocking), client sends, server replies.
        fn round_trip(
            request: &[u8],
            inbox: &mut [u8],
            response: &[u8],
            outbox: &mut [u8],
        ) -> Outcome {
            with_table(|table| {
                let (mut server_frame, mut client_frame) = (Frame::default(), Frame::default());
                let mut server_msg = Message::default();
                let server = receiving(table, &mut server_frame, &mut server_msg, inbox);
                let client = add(table, &mut client_frame, State::Running);
                as_caller(table, client);
                let mut msg = message(3, 4);
                let data = if request.is_empty() {
                    0
                } else {
                    request.as_ptr() as u32
                };
                check_eq!(
                    send(table, server, ptr(&mut msg), data, request.len() as u32),
                    0
                );
                // The reply's data lands in the client's data buffer (the
                // request's, for send_into); point it at `outbox`.
                table.get_mut(client).unwrap().data_ptr = if outbox.is_empty() {
                    0
                } else {
                    outbox.as_mut_ptr() as u32
                };
                table.get_mut(client).unwrap().data_len = outbox.len() as u32;
                check_eq!(server_msg, message(3, 4));
                check_eq!(server_frame.eax, client);
                check!(inbox[..request.len()] == *request);
                as_caller(table, server);
                let mut answer = message(5, 6);
                let data = if response.is_empty() {
                    0
                } else {
                    response.as_ptr() as u32
                };
                check_eq!(
                    reply(table, client, ptr(&mut answer), data, response.len() as u32),
                    0
                );
                check_eq!(msg, answer);
                check_eq!(client_frame.eax, 0);
                check!(outbox[..response.len()] == *response);
                Ok(())
            })
        }

        fn ipc_send_receive_blocked_with_data() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let (mut inbox, mut data) = (Message::default(), [0u8; 8]);
                let receiver = receiving(table, &mut rf, &mut inbox, &mut data);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(1, 1);
                send(table, receiver, ptr(&mut out), b"hello".as_ptr() as u32, 5);
                check_eq!(&data[..5], b"hello");
                Ok(())
            })
        }

        fn ipc_send_data_truncated() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let (mut inbox, mut data) = (Message::default(), [0u8; 4]);
                let receiver = receiving(table, &mut rf, &mut inbox, &mut data[..3]);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(1, 1);
                send(table, receiver, ptr(&mut out), b"hello".as_ptr() as u32, 5);
                check_eq!(data, *b"hel\0");
                Ok(())
            })
        }

        fn ipc_send_no_data() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let (mut inbox, mut data) = (Message::default(), [7u8; 4]);
                let receiver = receiving(table, &mut rf, &mut inbox, &mut data);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(2, 3);
                send(table, receiver, ptr(&mut out), 0, 0);
                check_eq!(inbox, out);
                check_eq!(data, [7; 4]);
                Ok(())
            })
        }

        fn ipc_reply_with_data() -> Outcome {
            with_table(|table| {
                let mut sf = Frame::default();
                let (mut reply_buf, mut data) = (Message::default(), [0u8; 8]);
                let sender = add(table, &mut sf, State::SendBlocked);
                let p = table.get_mut(sender).unwrap();
                p.msg_ptr = ptr(&mut reply_buf);
                p.data_ptr = data.as_mut_ptr() as u32;
                p.data_len = 8;
                let mut answer = message(1, 1);
                reply(table, sender, ptr(&mut answer), b"done".as_ptr() as u32, 4);
                check_eq!(&data[..4], b"done");
                Ok(())
            })
        }

        fn ipc_receive_queued_sender_with_data() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let receiver = add(table, &mut rf, State::Ready);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(1, 1);
                send(table, receiver, ptr(&mut out), b"queued".as_ptr() as u32, 6);
                as_caller(table, receiver);
                let (mut inbox, mut data) = (Message::default(), [0u8; 8]);
                let got = receive(table, ptr(&mut inbox), data.as_mut_ptr() as u32, 8);
                check_eq!(got, Received::Send(sender));
                check_eq!(&data[..6], b"queued");
                Ok(())
            })
        }

        fn notify_then_receive(capacity: usize) -> Result<Vec<u8>, crate::ktest::Failure> {
            with_table_result(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let receiver = add(table, &mut rf, State::Ready);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                let mut out = message(8, 9);
                let n = notify(
                    table,
                    receiver,
                    ptr(&mut out),
                    b"payload".as_ptr() as u32,
                    7,
                );
                check_eq!(n, 0);
                check_eq!(table.get(sender).unwrap().state, State::Running);
                as_caller(table, receiver);
                let mut inbox = Message::default();
                let mut data = alloc::vec![0u8; capacity];
                let got = receive(
                    table,
                    ptr(&mut inbox),
                    data.as_mut_ptr() as u32,
                    capacity as u32,
                );
                check_eq!(got, Received::Notification);
                check_eq!(inbox, out);
                Ok(data)
            })
        }

        fn with_table_result<T>(
            test: impl FnOnce(&mut Table) -> Result<T, crate::ktest::Failure>,
        ) -> Result<T, crate::ktest::Failure> {
            let before = snapshot();
            let result = PROCESSES.with(test);
            restore(&before);
            result
        }

        fn ipc_notify_queue_with_data() -> Outcome {
            check_eq!(notify_then_receive(16)?[..7], *b"payload");
            Ok(())
        }

        fn ipc_notify_queue_data_truncated() -> Outcome {
            check_eq!(notify_then_receive(3)?, b"pay");
            Ok(())
        }

        fn ipc_notify_to_receive_blocked_delivers_with_sender_0() -> Outcome {
            with_table(|table| {
                let (mut rf, mut sf) = (Frame::default(), Frame::default());
                let mut inbox = Message::default();
                let receiver = receiving(table, &mut rf, &mut inbox, &mut []);
                let sender = add(table, &mut sf, State::Running);
                as_caller(table, sender);
                rf.eax = 0xFFFF;
                let mut out = message(4, 4);
                check_eq!(notify(table, receiver, ptr(&mut out), 0, 0), 0);
                check_eq!((inbox, rf.eax), (out, 0));
                Ok(())
            })
        }

        /// A process with a heap at 0x00403000 in a fresh address space.
        fn with_heap(test: impl FnOnce(&mut Table, u32) -> Outcome) -> Outcome {
            with_table(|table| {
                let directory = create_address_space().unwrap_or(0);
                check!(directory != 0);
                let mut f = Frame::default();
                let pid = add(table, &mut f, State::Running);
                let p = table.get_mut(pid).unwrap();
                p.directory = directory;
                p.heap_base = 0x0040_3000;
                p.heap_break = 0x0040_3000;
                as_caller(table, pid);
                let outcome = test(table, directory);
                // Give the frames back; remove() leaves the rest to restore.
                table.get_mut(pid).unwrap().directory = 0;
                destroy_address_space(directory);
                outcome
            })
        }

        fn sbrk_returns_old_break() -> Outcome {
            with_heap(|table, _| {
                check_eq!(sbrk(table, 100), 0x0040_3000);
                check_eq!(table.current().heap_break, 0x0040_4000);
                Ok(())
            })
        }

        fn sbrk_zero_returns_current_break() -> Outcome {
            with_heap(|table, _| {
                check_eq!(sbrk(table, 0), 0x0040_3000);
                check_eq!(table.current().heap_break, 0x0040_3000);
                Ok(())
            })
        }

        fn sbrk_maps_multiple_pages() -> Outcome {
            with_heap(|table, directory| {
                check_eq!(sbrk(table, 3 * FRAME_SIZE), 0x0040_3000);
                for page in [0x0040_3000, 0x0040_4000, 0x0040_5000] {
                    check!(translate(directory, page).is_some());
                }
                check!(translate(directory, 0x0040_6000).is_none());
                Ok(())
            })
        }

        fn sbrk_no_heap_returns_zero() -> Outcome {
            with_table(|table| {
                let mut f = Frame::default();
                let pid = add(table, &mut f, State::Running);
                as_caller(table, pid);
                check_eq!(sbrk(table, 100), 0);
                Ok(())
            })
        }

        fn sbrk_overflow_returns_zero() -> Outcome {
            with_heap(|table, _| {
                check_eq!(sbrk(table, u32::MAX), 0);
                check_eq!(table.current().heap_break, 0x0040_3000);
                Ok(())
            })
        }

        fn sbrk_stack_collision_returns_zero() -> Outcome {
            with_heap(|table, _| {
                table.current().heap_break = USER_STACK_BOTTOM - FRAME_SIZE;
                check_eq!(sbrk(table, FRAME_SIZE + 1), 0);
                check_eq!(table.current().heap_break, USER_STACK_BOTTOM - FRAME_SIZE);
                Ok(())
            })
        }

        fn list(table: &Table, max: usize) -> Vec<ProcEntry> {
            let mut entries = alloc::vec![ProcEntry::default(); max];
            let n = proc_list(table, entries.as_mut_ptr() as u32, max as u32);
            entries.truncate(n as usize);
            entries
        }

        fn proclist_skips_kernel_task() -> Outcome {
            with_table(|table| {
                check!(list(table, 64).iter().all(|e| e.pid != KERNEL_PID));
                Ok(())
            })
        }

        fn proclist_includes_created_process() -> Outcome {
            with_table(|table| {
                let mut f = Frame::default();
                let pid = add(table, &mut f, State::Ready);
                table.get_mut(pid).unwrap().heap_base = 0x0040_3000;
                table.get_mut(pid).unwrap().heap_break = 0x0040_5000;
                let entry = list(table, 64).into_iter().find(|e| e.pid == pid);
                check_eq!(
                    entry,
                    Some(ProcEntry {
                        pid,
                        state: State::Ready as u32,
                        heap_size: 0x2000
                    })
                );
                Ok(())
            })
        }

        fn proclist_zero_heap_size_when_no_heap() -> Outcome {
            with_table(|table| {
                let mut f = Frame::default();
                let pid = add(table, &mut f, State::Ready);
                let entry = list(table, 64).into_iter().find(|e| e.pid == pid);
                check_eq!(entry.map(|e| e.heap_size), Some(0));
                Ok(())
            })
        }

        fn proclist_respects_max_entries() -> Outcome {
            with_table(|table| {
                let mut f = Frame::default();
                for _ in 0..3 {
                    add(table, &mut f, State::Ready);
                }
                check_eq!(list(table, 2).len(), 2);
                check_eq!(list(table, 0).len(), 0);
                Ok(())
            })
        }

        /// A syscall from ring 0 (buffers are trusted).
        fn raw_syscall(number: u32, ebx: u32, ecx: u32, edx: u32) -> u32 {
            let result: u32;
            unsafe {
                asm!(
                    "int 0x80",
                    inlateout("eax") number => result,
                    in("ebx") ebx,
                    in("ecx") ecx,
                    in("edx") edx,
                )
            };
            result
        }

        fn syscall_uptime_is_monotonic() -> Outcome {
            let first = raw_syscall(syscall::UPTIME, 0, 0, 0);
            pit::tick(0);
            let second = raw_syscall(syscall::UPTIME, 0, 0, 0);
            check!(second > first);
            Ok(())
        }

        fn syscall_write_to_other_fds_fails() -> Outcome {
            let text = b"x";
            for fd in [1, 99] {
                check_eq!(
                    raw_syscall(syscall::WRITE, fd, text.as_ptr() as u32, 1) as i32,
                    -1
                );
            }
            Ok(())
        }

        fn syscall_invalid_number_fails() -> Outcome {
            check_eq!(raw_syscall(999, 0, 0, 0) as i32, -1);
            check_eq!(
                raw_syscall(syscall::FRAMEBUFFER_INFO + 1, 0, 0, 0) as i32,
                -1
            );
            Ok(())
        }

        /// A process blocked in receive, added outside a borrow so the
        /// IRQ dispatch can take one.
        fn irq_receiver(frame: &mut Frame, buf: &mut Message, state: State) -> u32 {
            PROCESSES.with(|table| {
                let pid = add(table, frame, state);
                table.get_mut(pid).unwrap().msg_ptr = ptr(buf);
                pid
            })
        }

        const DISK: u8 = 14;

        fn irq_forward_delivers_to_receive_blocked_process() -> Outcome {
            let before = snapshot();
            let (mut f, mut inbox) = (Frame::default(), Message::default());
            f.eax = 0xFFFF;
            let pid = irq_receiver(&mut f, &mut inbox, State::ReceiveBlocked);
            irq::register_forward(DISK as u32, pid);
            irq::dispatch(irq::IRQ_OFFSET + DISK, 0);
            let state = PROCESSES.with(|t| t.get(pid).unwrap().state);
            irq::register_forward(DISK as u32, 0);
            restore(&before);
            check_eq!(state, State::Ready);
            check_eq!(
                (inbox.kind, inbox.arg1),
                (sys::ipc::message_type::IRQ_NOTIFY, DISK as u32)
            );
            check_eq!(f.eax, 0);
            Ok(())
        }

        fn irq_forward_sets_pending_when_not_receive_blocked() -> Outcome {
            let before = snapshot();
            let (mut f, mut inbox) = (Frame::default(), Message::default());
            let pid = irq_receiver(&mut f, &mut inbox, State::Ready);
            irq::register_forward(DISK as u32, pid);
            irq::dispatch(irq::IRQ_OFFSET + DISK, 0);
            // The next receive gets it.
            let got = PROCESSES.with(|table| {
                table.set_current(pid);
                let mut msg = Message::default();
                let got = receive(table, ptr(&mut msg), 0, 0);
                (got, msg.kind, msg.arg1)
            });
            irq::register_forward(DISK as u32, 0);
            restore(&before);
            check_eq!(
                got,
                (
                    Received::Notification,
                    sys::ipc::message_type::IRQ_NOTIFY,
                    DISK as u32
                )
            );
            Ok(())
        }

        fn irq_forward_coexists_with_the_pit() -> Outcome {
            let before = snapshot();
            let (mut f, mut inbox) = (Frame::default(), Message::default());
            let pid = irq_receiver(&mut f, &mut inbox, State::ReceiveBlocked);
            irq::register_forward(0, pid);
            let ticks = pit::ticks();
            irq::dispatch(irq::IRQ_OFFSET, 0);
            let ticked = pit::ticks() == ticks + 1;
            irq::register_forward(0, 0);
            restore(&before);
            check!(ticked);
            check_eq!(
                (inbox.kind, inbox.arg1),
                (sys::ipc::message_type::IRQ_NOTIFY, 0)
            );
            Ok(())
        }

        fn irq_forward_irq_takes_priority_over_send_queue() -> Outcome {
            let before = snapshot();
            let (mut rf, mut inbox) = (Frame::default(), Message::default());
            let receiver = irq_receiver(&mut rf, &mut inbox, State::Ready);
            let mut sf = Frame::default();
            PROCESSES.with(|table| {
                let sender = add(table, &mut sf, State::SendBlocked);
                table.get_mut(receiver).unwrap().push_sender(sender);
            });
            irq::register_forward(DISK as u32, receiver);
            irq::dispatch(irq::IRQ_OFFSET + DISK, 0);
            let got = PROCESSES.with(|table| {
                table.set_current(receiver);
                let mut msg = Message::default();
                (receive(table, ptr(&mut msg), 0, 0), msg.kind)
            });
            irq::register_forward(DISK as u32, 0);
            restore(&before);
            check_eq!(
                got,
                (Received::Notification, sys::ipc::message_type::IRQ_NOTIFY)
            );
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_break_grows_to_a_page_boundary() {
        assert_eq!(grown_break(0x0040_3000, 1), Some(0x0040_4000));
        assert_eq!(grown_break(0x0040_3000, 0x1000), Some(0x0040_4000));
        assert_eq!(grown_break(0x0040_3000, 0x1001), Some(0x0040_5000));
        assert_eq!(grown_break(0x0040_3000, 0), Some(0x0040_3000));
    }

    #[test]
    fn the_break_stops_below_the_stack_and_never_wraps() {
        assert_eq!(
            grown_break(USER_STACK_BOTTOM - 0x1000, 0x1000),
            Some(USER_STACK_BOTTOM)
        );
        assert_eq!(grown_break(USER_STACK_BOTTOM - 0x1000, 0x1001), None);
        assert_eq!(grown_break(0x0040_0000, u32::MAX), None);
        assert_eq!(grown_break(0xFFFF_F000, 0x2000), None);
    }

    #[test]
    fn buffers_must_end_below_kernel_space() {
        assert!(below_kernel(0x1000, 0x1000));
        assert!(below_kernel(KERNEL_VBASE - 4, 4));
        assert!(!below_kernel(KERNEL_VBASE - 4, 5));
        assert!(!below_kernel(0xFFFF_F000, 0x2000));
    }
}
