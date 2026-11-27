// scheduler.rs -- round-robin scheduling
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The timer moves the current process to the back after TIME_SLICE
// ticks; a blocking syscall gives up the CPU at once. Switching saves the
// kernel stack pointer of the process leaving, then loads the next one's
// address space, kernel stack (TSS.esp0) and saved frame. The kernel task
// runs when nothing else is ready.

use core::arch::asm;

use crate::memory::paging::kernel_directory;
use crate::system::gdt::set_kernel_stack;
use crate::system::process::{PROCESSES, State, TIME_SLICE, Table};

/// Make `next` the running process. Returns the frame to resume.
fn switch_to(table: &mut Table, next: u32, esp: u32) -> u32 {
    let current = table.current_pid();
    table.current().esp = esp;
    let process = table.get_mut(next).expect("the next process exists");
    process.state = State::Running;
    // A process without its own directory (the kernel task) runs in the
    // kernel's: keeping the last one loaded would leave CR3 on a directory
    // that exit_current is about to free.
    let directory = match process.directory {
        0 => kernel_directory(),
        directory => directory,
    };
    let loaded: u32;
    unsafe { asm!("mov {}, cr3", out(reg) loaded, options(nomem, nostack)) };
    if directory != loaded {
        // SAFETY: every directory shares the kernel half.
        unsafe { asm!("mov cr3, {}", in(reg) directory, options(nostack)) };
    }
    if process.kernel_stack_top != 0 {
        set_kernel_stack(process.kernel_stack_top);
    }
    let resume = process.esp;
    if next != current {
        table.set_current(next);
    }
    table.ticks = 0;
    resume
}

/// A timer tick: after TIME_SLICE ticks, move on to the next ready
/// process. Returns the frame to resume.
pub fn schedule(table: &mut Table, esp: u32) -> u32 {
    if table.len() <= 1 {
        return esp;
    }
    table.ticks += 1;
    if table.ticks < TIME_SLICE {
        return esp;
    }
    // A blocked process keeps its state; only a running one is demoted.
    let current = table.current();
    if current.state == State::Running {
        current.state = State::Ready;
    }
    let next = table.next_ready(table.current_pid());
    switch_to(table, next, esp)
}

/// Give up the CPU now, after the caller has set the current process's
/// state (blocked). Returns the frame to resume.
pub fn reschedule(table: &mut Table, esp: u32) -> u32 {
    if table.len() <= 1 {
        return esp;
    }
    let next = table.next_ready(table.current_pid());
    switch_to(table, next, esp)
}

/// End the current process and switch to the next ready one, or to the
/// kernel task. Returns the frame to resume.
pub fn exit_current(table: &mut Table, esp: u32) -> u32 {
    let dying = table.current_pid();
    let mut next = table.next_ready(dying);
    if next == dying {
        next = crate::system::process::KERNEL_PID;
    }
    // Switch first, so destroying the dying process's stack is safe: it is
    // not resumed again.
    let resume = switch_to(table, next, esp);
    table.destroy(dying);
    resume
}

/// The PIT's entry: a tick.
pub fn tick(esp: u32) -> u32 {
    PROCESSES.with(|table| schedule(table, esp))
}

#[cfg(feature = "ktest")]
pub mod ktests {
    use super::*;
    use crate::ktest::Outcome;
    use crate::system::process::testing::{add, with_table};
    use crate::system::process::{Frame, KERNEL_PID};

    tests!(
        scheduler_single_process_returns_same_esp,
        scheduler_two_processes_switch_after_time_slice,
        scheduler_skips_send_blocked,
        scheduler_skips_receive_blocked,
        scheduler_reschedule_switches_to_ready,
        scheduler_exit_current_wakes_waiters,
    );

    fn scheduler_single_process_returns_same_esp() -> Outcome {
        with_table(|table| {
            check_eq!(table.len(), 1);
            check_eq!(schedule(table, 0x1234), 0x1234);
            Ok(())
        })
    }

    fn scheduler_two_processes_switch_after_time_slice() -> Outcome {
        with_table(|table| {
            let mut frame = Frame::default();
            let pid = add(table, &mut frame, State::Ready);
            for _ in 1..TIME_SLICE {
                check_eq!(schedule(table, 0x1234), 0x1234);
            }
            check_eq!(schedule(table, 0x1234), &frame as *const Frame as u32);
            check_eq!(table.current_pid(), pid);
            check_eq!(table.get(KERNEL_PID).unwrap().state, State::Ready);
            check_eq!(table.get(KERNEL_PID).unwrap().esp, 0x1234);
            Ok(())
        })
    }

    fn scheduler_skips_send_blocked() -> Outcome {
        scheduler_skips(State::SendBlocked)
    }

    fn scheduler_skips_receive_blocked() -> Outcome {
        scheduler_skips(State::ReceiveBlocked)
    }

    fn scheduler_skips(blocked: State) -> Outcome {
        with_table(|table| {
            let (mut a, mut b) = (Frame::default(), Frame::default());
            add(table, &mut a, blocked);
            let ready = add(table, &mut b, State::Ready);
            table.ticks = TIME_SLICE - 1;
            check_eq!(schedule(table, 0x1234), &b as *const Frame as u32);
            check_eq!(table.current_pid(), ready);
            Ok(())
        })
    }

    fn scheduler_reschedule_switches_to_ready() -> Outcome {
        with_table(|table| {
            let mut frame = Frame::default();
            let pid = add(table, &mut frame, State::Ready);
            table.current().state = State::SendBlocked;
            check_eq!(reschedule(table, 0x1234), &frame as *const Frame as u32);
            check_eq!(table.current_pid(), pid);
            check_eq!(table.get(KERNEL_PID).unwrap().state, State::SendBlocked);
            Ok(())
        })
    }

    fn scheduler_exit_current_wakes_waiters() -> Outcome {
        with_table(|table| {
            let (mut dying, mut waiter) = (Frame::default(), Frame::default());
            let dying_pid = add(table, &mut dying, State::Running);
            let waiter_pid = add(table, &mut waiter, State::WaitBlocked);
            table.get_mut(waiter_pid).unwrap().wait_pid = dying_pid;
            waiter.eax = 0xFFFF;
            table.set_current(dying_pid);
            table.get_mut(KERNEL_PID).unwrap().state = State::Ready;
            table.get_mut(KERNEL_PID).unwrap().esp = 0x5678;
            let resume = exit_current(table, 0x1234);
            check!(table.get(dying_pid).is_none());
            // The next process is chosen before the waiter is woken (with
            // 0), so the kernel task runs first.
            check_eq!(resume, 0x5678);
            check_eq!(table.current_pid(), KERNEL_PID);
            check_eq!(waiter.eax, 0);
            check_eq!(table.get(waiter_pid).unwrap().state, State::Ready);
            Ok(())
        })
    }
}
