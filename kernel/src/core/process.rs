// process.rs -- processes and the process table
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Every process is a row of the table: its state, the kernel stack frame
// it resumes from, its address space and heap, and its IPC state (the
// message it is sending or receiving, and the queues of senders and
// notifications waiting for it). PID 0 is the kernel task: the idle loop
// in start(), always there to run when nothing else is ready.
//
// The table lives in one Global, borrowed once per kernel entry (syscall,
// timer tick, IRQ, exception). A second process is reached by its pid,
// never through a reference held across the borrow.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

use alloc::collections::VecDeque;
use alloc::vec::Vec;

use sys::ipc::Message;

pub const KERNEL_PID: u32 = 0;

/// Values as reported by proc_list (sys::os::ProcEntry::state).
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum State {
    Ready = 1,
    Running = 2,
    SendBlocked = 3,
    ReceiveBlocked = 4,
    WaitBlocked = 5,
}

/// The registers an entry stub saves on the kernel stack (stub.s,
/// syscall_stub.s), lowest address first. `user_esp` and `user_ss` are
/// only there when the CPU came from ring 3.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Frame {
    pub gs: u32,
    pub fs: u32,
    pub es: u32,
    pub ds: u32,
    pub edi: u32,
    pub esi: u32,
    pub ebp: u32,
    pub esp_pusha: u32,
    pub ebx: u32,
    pub edx: u32,
    pub ecx: u32,
    pub eax: u32,
    pub number: u32,
    pub error_code: u32,
    pub eip: u32,
    pub cs: u32,
    pub eflags: u32,
    pub user_esp: u32,
    pub user_ss: u32,
}

impl Frame {
    /// Whether the CPU entered the kernel from user mode.
    pub fn from_user(&self) -> bool {
        self.cs & 3 == 3
    }
}

/// A fire-and-forget message waiting to be received, with a copy of its
/// data (the sender's buffer may be gone by then).
#[derive(Debug)]
pub struct Notification {
    pub msg: Message,
    pub data: Vec<u8>,
}

#[derive(Debug)]
pub struct Process {
    pub pid: u32,
    pub state: State,
    /// Kernel stack pointer to resume from: it points at a Frame.
    pub esp: u32,
    /// Physical address of the page directory; 0 shares the kernel's.
    pub directory: u32,
    /// Top of the kernel stack (TSS.esp0); 0 for the kernel task.
    pub kernel_stack_top: u32,
    /// User code and data selectors, for the programs it execs.
    pub cs: u32,
    pub ds: u32,
    /// The heap as first loaded, and its current end (0 without a heap).
    pub heap_base: u32,
    pub heap_break: u32,
    /// The process a WaitBlocked process waits for, or a SendBlocked one
    /// sent to (queued or waiting for the reply).
    pub wait_pid: u32,
    /// The message being sent (SendBlocked) or the user buffer a reply
    /// or received message goes to.
    pub msg: Message,
    pub msg_ptr: u32,
    /// User buffer for bulk data: outgoing (sending) or incoming
    /// (receiving), with its length or capacity.
    pub data_ptr: u32,
    pub data_len: u32,
    /// Processes blocked sending to this one, in order.
    senders: VecDeque<u32>,
    notifications: VecDeque<Notification>,
    /// Bytes of data in `notifications`.
    notification_bytes: usize,
}

impl Process {
    fn new(pid: u32, esp: u32, directory: u32, cs: u32, ds: u32) -> Self {
        Self {
            pid,
            state: State::Ready,
            esp,
            directory,
            kernel_stack_top: 0,
            cs,
            ds,
            heap_base: 0,
            heap_break: 0,
            wait_pid: 0,
            msg: Message::default(),
            msg_ptr: 0,
            data_ptr: 0,
            data_len: 0,
            senders: VecDeque::new(),
            notifications: VecDeque::new(),
            notification_bytes: 0,
        }
    }

    /// Queue `sender` as blocked sending to this process. False if memory
    /// is exhausted.
    pub fn push_sender(&mut self, sender: u32) -> bool {
        if self.senders.try_reserve(1).is_err() {
            return false;
        }
        self.senders.push_back(sender);
        true
    }

    /// Queue a notification, copying `data`. False if the queue is full
    /// (MAX_NOTIFICATIONS, MAX_NOTIFICATION_DATA) or memory is exhausted.
    pub fn push_notification(&mut self, msg: Message, data: &[u8]) -> bool {
        if self.notifications.len() >= MAX_NOTIFICATIONS
            || self.notification_bytes + data.len() > MAX_NOTIFICATION_DATA
        {
            return false;
        }
        let mut copy = Vec::new();
        if copy.try_reserve_exact(data.len()).is_err() || self.notifications.try_reserve(1).is_err()
        {
            return false;
        }
        copy.extend_from_slice(data);
        self.notification_bytes += copy.len();
        self.notifications
            .push_back(Notification { msg, data: copy });
        true
    }

    /// Take the oldest notification. An emptied queue gives its buffer
    /// back: a full-screen redraw grows the display's to about 300 KiB.
    pub fn pop_notification(&mut self) -> Option<Notification> {
        let notification = self.notifications.pop_front()?;
        self.notification_bytes -= notification.data.len();
        if self.notifications.is_empty() {
            self.notifications = VecDeque::new();
        }
        Some(notification)
    }

    /// Take the first blocked sender. An emptied queue gives its buffer
    /// back.
    pub fn pop_sender(&mut self) -> Option<u32> {
        let sender = self.senders.pop_front()?;
        if self.senders.is_empty() {
            self.senders = VecDeque::new();
        }
        Some(sender)
    }

    /// The first blocked sender, if any.
    pub fn first_sender(&self) -> Option<u32> {
        self.senders.front().copied()
    }

    /// Bytes of heap grown by sbrk.
    pub fn heap_size(&self) -> u32 {
        self.heap_break.saturating_sub(self.heap_base)
    }
}

/// The process table and the scheduler's state.
pub struct Table {
    /// In creation order; the kernel task first.
    processes: Vec<Process>,
    current: u32,
    next_pid: u32,
    /// Timer ticks the current process has run since it was chosen.
    pub ticks: u32,
}

/// Most notifications a process may have waiting, and most bytes of data
/// in them: the kernel heap is 1 MiB, and no sender may fill it. The count
/// covers a full-screen redraw, which the terminal sends to the display as
/// one notification per character (8192 entries are about 300 KiB).
pub const MAX_NOTIFICATIONS: usize = 8192;
pub const MAX_NOTIFICATION_DATA: usize = 64 * 1024;

/// Ticks a process runs before the scheduler may switch.
pub const TIME_SLICE: u32 = 10;

impl Table {
    pub const fn new() -> Self {
        Self {
            processes: Vec::new(),
            current: KERNEL_PID,
            next_pid: 1,
            ticks: 0,
        }
    }

    /// Add the kernel task, running. Does nothing if it is there.
    pub fn init(&mut self) {
        if self.processes.is_empty() {
            let mut kernel = Process::new(KERNEL_PID, 0, 0, 0, 0);
            kernel.state = State::Running;
            self.processes.push(kernel);
        }
    }

    /// A new Ready process resuming from `esp`; its pid, or None if
    /// memory is exhausted.
    pub fn create(&mut self, esp: u32, directory: u32, cs: u32, ds: u32) -> Option<u32> {
        self.processes.try_reserve(1).ok()?;
        let pid = self.next_pid;
        self.next_pid += 1;
        self.processes
            .push(Process::new(pid, esp, directory, cs, ds));
        Some(pid)
    }

    /// Take `pid` out of the table (not the kernel task).
    pub fn remove(&mut self, pid: u32) -> Option<Process> {
        if pid == KERNEL_PID {
            return None;
        }
        let index = self.index(pid)?;
        Some(self.processes.remove(index))
    }

    fn index(&self, pid: u32) -> Option<usize> {
        self.processes.iter().position(|p| p.pid == pid)
    }

    pub fn get(&self, pid: u32) -> Option<&Process> {
        self.processes.iter().find(|p| p.pid == pid)
    }

    pub fn get_mut(&mut self, pid: u32) -> Option<&mut Process> {
        self.processes.iter_mut().find(|p| p.pid == pid)
    }

    pub fn current_pid(&self) -> u32 {
        self.current
    }

    pub fn set_current(&mut self, pid: u32) {
        self.current = pid;
    }

    pub fn current(&mut self) -> &mut Process {
        let pid = self.current;
        self.get_mut(pid)
            .expect("the current process is in the table")
    }

    pub fn len(&self) -> usize {
        self.processes.len()
    }

    pub fn iter(&self) -> impl Iterator<Item = &Process> {
        self.processes.iter()
    }

    pub fn iter_mut(&mut self) -> impl Iterator<Item = &mut Process> {
        self.processes.iter_mut()
    }

    /// The next Ready process after `pid` in table order, wrapping around
    /// to `pid` itself last. The kernel task (the idle loop) only when no
    /// other process is ready, and `pid` if not even it is.
    pub fn next_ready(&self, pid: u32) -> u32 {
        let start = self.index(pid).unwrap_or(0);
        let count = self.processes.len();
        let ready = |p: &&Process| p.state == State::Ready;
        let round = (1..=count).map(|offset| &self.processes[(start + offset) % count]);
        round
            .clone()
            .filter(|p| p.pid != KERNEL_PID)
            .find(ready)
            .or_else(|| round.filter(|p| p.pid == KERNEL_PID).find(ready))
            .map_or(pid, |p| p.pid)
    }
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use super::*;
    use crate::memory::paging::{
        PAGE_PRESENT, PAGE_READWRITE, PAGE_USER, destroy_address_space, map_user_page,
    };
    use crate::memory::physical::{FRAME_SIZE, PHYSICAL};
    use crate::memory::{KERNEL_VBASE, phys_to_virt};
    use crate::sync::Global;

    pub static PROCESSES: Global<Table> = Global::new(Table::new());

    /// Top of a new process's user stack, one page below it.
    pub const USER_STACK_TOP: u32 = KERNEL_VBASE;
    /// IF set, IOPL 3 (drivers use port I/O from user mode).
    const USER_EFLAGS: u32 = 0x3202;

    /// The frame a blocked process resumes from, if `esp` points into the
    /// kernel (tests build processes with no frame at all).
    pub fn frame_of(esp: u32) -> Option<*mut Frame> {
        (esp >= KERNEL_VBASE).then_some(esp as *mut Frame)
    }

    /// Set the value a blocked process's syscall returns when it resumes.
    pub fn set_return(process: &Process, value: u32) {
        if let Some(frame) = frame_of(process.esp) {
            // SAFETY: a blocked process's esp points at the frame its
            // syscall saved, on its own kernel stack.
            unsafe { (*frame).eax = value };
        }
    }

    /// Add the kernel task (C++ start() and the test kernel).
    #[unsafe(no_mangle)]
    extern "C" fn process_init() {
        PROCESSES.with(|table| table.init());
    }

    /// Start a loaded module (C++ start()): the new pid, or 0.
    #[unsafe(no_mangle)]
    extern "C" fn process_spawn(
        directory: u32,
        entry: u32,
        heap_start: u32,
        cs: u32,
        ds: u32,
    ) -> u32 {
        PROCESSES
            .with(|table| table.spawn(directory, entry, heap_start, cs, ds))
            .unwrap_or(0)
    }

    impl Table {
        /// Start a program loaded into `directory`: map its user stack,
        /// give it a kernel stack with a frame that enters user mode at
        /// `entry`, and add it Ready. Its pid, or None (then nothing is
        /// left allocated but `directory`, which the caller destroys).
        pub fn spawn(
            &mut self,
            directory: u32,
            entry: u32,
            heap_start: u32,
            cs: u32,
            ds: u32,
        ) -> Option<u32> {
            let user_stack = PHYSICAL.with(|p| p.alloc())?;
            let flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
            // Destroying the address space frees the stack frame with it.
            map_user_page(directory, USER_STACK_TOP - FRAME_SIZE, user_stack, flags);

            let kernel_stack = PHYSICAL.with(|p| p.alloc())?;
            let top = phys_to_virt(kernel_stack) as u32 + FRAME_SIZE;
            let esp = top - size_of::<Frame>() as u32;
            let frame = Frame {
                gs: ds,
                fs: ds,
                es: ds,
                ds,
                eip: entry,
                cs,
                eflags: USER_EFLAGS,
                user_esp: USER_STACK_TOP,
                user_ss: ds,
                ..Frame::default()
            };
            // SAFETY: the frame fits at the top of the fresh stack frame.
            unsafe { (esp as *mut Frame).write(frame) };

            let Some(pid) = self.create(esp, directory, cs, ds) else {
                PHYSICAL.with(|p| p.free(kernel_stack));
                return None;
            };
            let process = self.get_mut(pid).unwrap();
            process.kernel_stack_top = top;
            process.heap_base = heap_start;
            process.heap_break = heap_start;
            Some(pid)
        }

        /// End `pid`: fail every send blocked on it, wake every process
        /// waiting for it, and free its queues, address space and kernel
        /// stack. The caller must not be running on that stack afterwards.
        pub fn destroy(&mut self, pid: u32) {
            let Some(process) = self.remove(pid) else {
                return;
            };
            // Senders fail with -1, whether still queued or already
            // delivered and waiting for the reply; waiters succeed with 0.
            for other in self.iter_mut() {
                if other.wait_pid != pid {
                    continue;
                }
                match other.state {
                    State::SendBlocked => set_return(other, u32::MAX),
                    State::WaitBlocked => set_return(other, 0),
                    _ => continue,
                }
                other.state = State::Ready;
            }
            if process.directory != 0 {
                destroy_address_space(process.directory);
            }
            if process.kernel_stack_top != 0 {
                let stack = process.kernel_stack_top - KERNEL_VBASE - FRAME_SIZE;
                PHYSICAL.with(|p| p.free(stack));
            }
        }
    }

    /// Helpers for kernel tests that build processes.
    #[cfg(not(test))]
    pub mod testing {
        use alloc::vec::Vec;

        use super::*;

        /// A process resuming from `frame` (on the kernel stack), with no
        /// address space of its own.
        pub fn add(table: &mut Table, frame: &mut Frame, state: State) -> u32 {
            let pid = table
                .create(frame as *mut Frame as u32, 0, 0x08, 0x10)
                .unwrap();
            table.get_mut(pid).unwrap().state = state;
            pid
        }

        /// The pids in the table now, to restore it with `restore`.
        pub fn snapshot() -> Vec<u32> {
            PROCESSES.with(|table| table.iter().map(|p| p.pid).collect())
        }

        /// Remove every process not in `before` and make the kernel task
        /// current and running again.
        pub fn restore(before: &[u32]) {
            PROCESSES.with(|table| {
                let added: Vec<u32> = table
                    .iter()
                    .map(|p| p.pid)
                    .filter(|pid| !before.contains(pid))
                    .collect();
                for pid in added {
                    table.remove(pid);
                }
                table.set_current(KERNEL_PID);
                table.current().state = State::Running;
                table.ticks = 0;
            });
        }

        /// Run `test` on the table, then `restore` it.
        pub fn with_table(
            test: impl FnOnce(&mut Table) -> crate::ktest::Outcome,
        ) -> crate::ktest::Outcome {
            let before = snapshot();
            let outcome = PROCESSES.with(test);
            restore(&before);
            outcome
        }
    }

    #[cfg(not(test))]
    pub mod ktests {
        use super::testing::{add, with_table};
        use super::*;
        use crate::ktest::Outcome;

        tests!(
            process_create_assigns_pid,
            process_create_unique_pids,
            process_create_beyond_old_limit,
            process_destroy_frees_slot,
            process_current_returns_kernel_task,
            process_get_returns_none_for_nonexistent,
            process_destroy_wakes_send_blocked_senders,
            process_spawn_builds_a_user_mode_frame,
        );

        fn process_create_assigns_pid() -> Outcome {
            with_table(|table| {
                let mut frame = Frame::default();
                let pid = add(table, &mut frame, State::Ready);
                check!(pid != KERNEL_PID);
                check_eq!(table.get(pid).map(|p| p.state), Some(State::Ready));
                Ok(())
            })
        }

        fn process_create_unique_pids() -> Outcome {
            with_table(|table| {
                let (mut a, mut b) = (Frame::default(), Frame::default());
                check!(add(table, &mut a, State::Ready) != add(table, &mut b, State::Ready));
                Ok(())
            })
        }

        fn process_create_beyond_old_limit() -> Outcome {
            // The C++ table once held 16 processes.
            with_table(|table| {
                let mut frame = Frame::default();
                for _ in 0..40 {
                    add(table, &mut frame, State::Ready);
                }
                check_eq!(table.len(), 41);
                Ok(())
            })
        }

        fn process_destroy_frees_slot() -> Outcome {
            with_table(|table| {
                let mut frame = Frame::default();
                let pid = add(table, &mut frame, State::Ready);
                let len = table.len();
                table.destroy(pid);
                check!(table.get(pid).is_none());
                check_eq!(table.len(), len - 1);
                Ok(())
            })
        }

        fn process_current_returns_kernel_task() -> Outcome {
            with_table(|table| {
                check_eq!(table.current_pid(), KERNEL_PID);
                check_eq!(table.current().state, State::Running);
                Ok(())
            })
        }

        fn process_get_returns_none_for_nonexistent() -> Outcome {
            with_table(|table| {
                check!(table.get(9999).is_none());
                Ok(())
            })
        }

        fn process_destroy_wakes_send_blocked_senders() -> Outcome {
            with_table(|table| {
                let (mut target, mut queued, mut answered) =
                    (Frame::default(), Frame::default(), Frame::default());
                let target_pid = add(table, &mut target, State::Ready);
                // One still in the queue, one delivered and awaiting the reply.
                let queued_pid = add(table, &mut queued, State::SendBlocked);
                let answered_pid = add(table, &mut answered, State::SendBlocked);
                table.get_mut(target_pid).unwrap().push_sender(queued_pid);
                for pid in [queued_pid, answered_pid] {
                    table.get_mut(pid).unwrap().wait_pid = target_pid;
                }
                table.destroy(target_pid);
                // Both sends fail with -1.
                check_eq!((queued.eax, answered.eax), (u32::MAX, u32::MAX));
                for pid in [queued_pid, answered_pid] {
                    check_eq!(table.get(pid).unwrap().state, State::Ready);
                }
                Ok(())
            })
        }

        fn process_spawn_builds_a_user_mode_frame() -> Outcome {
            with_table(|table| {
                let free = PHYSICAL.with(|p| p.free_frames());
                let directory = crate::memory::paging::create_address_space().unwrap_or(0);
                check!(directory != 0);
                let pid = table.spawn(directory, 0x0040_0000, 0x0040_3000, 0x1B, 0x23);
                check!(pid.is_some());
                let process = table.get(pid.unwrap()).unwrap();
                let frame = unsafe { *(process.esp as *const Frame) };
                check_eq!(
                    (frame.eip, frame.cs, frame.eflags),
                    (0x0040_0000, 0x1B, 0x3202)
                );
                check_eq!(
                    (frame.user_esp, frame.user_ss, frame.ds),
                    (USER_STACK_TOP, 0x23, 0x23)
                );
                check_eq!(
                    process.esp + size_of::<Frame>() as u32,
                    process.kernel_stack_top
                );
                check_eq!(
                    (process.heap_base, process.heap_break),
                    (0x0040_3000, 0x0040_3000)
                );
                // Destroying it gives back the directory, the stack table and
                // both stacks.
                table.destroy(pid.unwrap());
                check_eq!(PHYSICAL.with(|p| p.free_frames()), free);
                Ok(())
            })
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::mem::offset_of;

    #[test]
    fn the_frame_matches_the_stubs() {
        // pusha's eax lands at 44 after the four segment registers; the
        // CPU's eip and cs follow number and error_code.
        assert_eq!(offset_of!(Frame, eax), 44);
        assert_eq!(offset_of!(Frame, number), 48);
        assert_eq!(offset_of!(Frame, eip), 56);
        assert_eq!(offset_of!(Frame, cs), 60);
        assert_eq!(offset_of!(Frame, user_ss), 72);
        assert_eq!(size_of::<Frame>(), 76);
    }

    #[test]
    fn user_mode_frames_have_ring_3_selectors() {
        let frame = |cs| Frame {
            cs,
            ..Frame::default()
        };
        assert!(frame(0x1B).from_user());
        assert!(!frame(0x08).from_user());
    }

    fn table(states: &[State]) -> Table {
        let mut table = Table::new();
        table.init();
        for &state in states {
            let pid = table.create(0, 0, 0, 0).unwrap();
            table.get_mut(pid).unwrap().state = state;
        }
        table
    }

    #[test]
    fn pids_are_unique_and_never_reused() {
        let mut t = table(&[State::Ready, State::Ready]);
        assert_eq!(t.iter().map(|p| p.pid).collect::<Vec<_>>(), [0, 1, 2]);
        assert!(t.remove(2).is_some());
        assert_eq!(t.create(0, 0, 0, 0), Some(3));
        assert!(t.get(2).is_none());
    }

    #[test]
    fn the_kernel_task_cannot_be_removed() {
        let mut t = table(&[]);
        assert!(t.remove(KERNEL_PID).is_none());
        assert_eq!(t.current_pid(), KERNEL_PID);
        assert_eq!(t.current().state, State::Running);
    }

    #[test]
    fn next_ready_goes_round_and_skips_blocked_processes() {
        use State::*;
        let t = table(&[Ready, SendBlocked, ReceiveBlocked, Ready, WaitBlocked]);
        assert_eq!(t.next_ready(0), 1);
        assert_eq!(t.next_ready(1), 4);
        // Wraps past the end to pid 1.
        assert_eq!(t.next_ready(4), 1);
    }

    #[test]
    fn the_idle_kernel_task_runs_only_when_nothing_else_is_ready() {
        use State::*;
        let mut t = table(&[Ready, Ready]);
        t.get_mut(KERNEL_PID).unwrap().state = Ready;
        // From 2 the kernel task is next in order, but pid 1 comes first.
        assert_eq!(t.next_ready(2), 1);
        // A process the scheduler just made Ready keeps the CPU over idle.
        t.get_mut(1).unwrap().state = SendBlocked;
        assert_eq!(t.next_ready(2), 2);
        t.get_mut(2).unwrap().state = SendBlocked;
        assert_eq!(t.next_ready(2), KERNEL_PID);
    }

    #[test]
    fn notification_queues_are_capped() {
        let mut p = Process::new(1, 0, 0, 0, 0);
        for _ in 0..MAX_NOTIFICATIONS {
            assert!(p.push_notification(Message::default(), &[]));
        }
        assert!(!p.push_notification(Message::default(), &[]));
        let mut p = Process::new(1, 0, 0, 0, 0);
        let half = [0u8; MAX_NOTIFICATION_DATA / 2];
        assert!(p.push_notification(Message::default(), &half));
        assert!(p.push_notification(Message::default(), &half));
        assert!(!p.push_notification(Message::default(), &[0]));
        // Taking one off makes room again.
        assert_eq!(p.pop_notification().map(|n| n.data.len()), Some(half.len()));
        assert!(p.push_notification(Message::default(), &half));
        assert!(!p.push_notification(Message::default(), &[0]));
        // Draining the queue releases its buffer.
        while p.pop_notification().is_some() {}
        assert_eq!(p.notifications.capacity(), 0);
        assert!(p.push_notification(Message::default(), &half));
    }

    #[test]
    fn next_ready_stays_put_when_nothing_else_is_ready() {
        let t = table(&[State::SendBlocked]);
        assert_eq!(t.next_ready(0), 0);
        let only = table(&[]);
        assert_eq!(only.next_ready(0), 0);
    }

    #[test]
    fn senders_queue_in_order_past_the_old_limit() {
        // The C++ queue once held 8, then 32 entries.
        let mut p = Process::new(1, 0, 0, 0, 0);
        for sender in 2..42 {
            assert!(p.push_sender(sender));
        }
        assert!(p.senders.iter().copied().eq(2..42));
    }

    #[test]
    fn senders_interleave_push_and_pop() {
        let mut p = Process::new(1, 0, 0, 0, 0);
        p.push_sender(2);
        p.push_sender(3);
        assert_eq!(p.pop_sender(), Some(2));
        p.push_sender(4);
        assert_eq!(p.pop_sender(), Some(3));
        assert_eq!(p.pop_sender(), Some(4));
        assert_eq!(p.pop_sender(), None);
        assert_eq!(p.senders.capacity(), 0);
    }

    #[test]
    fn notifications_queue_in_order_with_a_copy_of_their_data() {
        let mut p = Process::new(1, 0, 0, 0, 0);
        for kind in 0..40 {
            let msg = Message {
                kind,
                ..Message::default()
            };
            assert!(p.push_notification(msg, &[kind as u8; 3]));
        }
        let kinds: Vec<u32> = p.notifications.iter().map(|n| n.msg.kind).collect();
        assert!(kinds.iter().copied().eq(0..40));
        assert_eq!(p.notifications[5].data, [5, 5, 5]);
        assert!(p.push_notification(Message::default(), &[]));
        assert!(p.notifications.back().unwrap().data.is_empty());
    }

    #[test]
    fn heap_size_is_what_sbrk_added() {
        let mut p = Process::new(1, 0, 0, 0, 0);
        assert_eq!(p.heap_size(), 0);
        p.heap_base = 0x0040_3000;
        p.heap_break = 0x0040_5000;
        assert_eq!(p.heap_size(), 0x2000);
    }
}
