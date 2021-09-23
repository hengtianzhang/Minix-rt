/*
 * Based on arch/arm/kernel/signal.c
 *
 * Copyright (C) 1995-2009 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <base/linkage.h>
#include <base/common.h>

#include <sel4m/syscalls.h>
#include <sel4m/sched.h>
#include <sel4m/signal.h>

#include <asm/esr.h>
#include <asm/daifflags.h>

struct rt_sigframe {
	//struct siginfo info;
	//struct ucontext uc;
};

struct frame_record {
	u64 fp;
	u64 lr;
};

struct rt_sigframe_user_layout {
	struct rt_sigframe __user *sigframe;
	struct frame_record __user *next_frame;

	unsigned long size;	/* size of allocated sigframe data */
	unsigned long limit;	/* largest allowed size */

	unsigned long fpsimd_offset;
	unsigned long esr_offset;
	unsigned long sve_offset;
	unsigned long extra_offset;
	unsigned long end_offset;
};

static void setup_restart_syscall(struct pt_regs *regs)
{
	BUG_ON(1);
	/* TODO */
}

static int setup_rt_frame(int usig, struct ksignal *ksig, struct pt_regs *regs)
{
	return 0;
}

/*
 * OK, we're invoking a handler
 */
static void handle_signal(struct ksignal *ksig, struct pt_regs *regs)
{
	//struct task_struct *tsk = current;
	int usig = ksig->sig;
	int ret;

	ret = setup_rt_frame(usig, ksig, regs);
	printf("handle \n");
}

static void do_signal(struct pt_regs *regs)
{
	unsigned long continue_addr = 0, restart_addr = 0;
	int retval = 0;
	struct ksignal ksig;
	bool syscall = in_syscall(regs);

	if (syscall) {
		continue_addr = regs->pc;
		restart_addr = continue_addr - 4;
		retval = regs->regs[0];

		/*
		 * Avoid additional syscall restarting via ret_to_user.
		 */
		forget_syscall(regs);

		/*
		 * Prepare for system call restart. We do this here so that a
		 * debugger will see the already changed PC.
		 */
		switch (retval) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
		case -ERESTART_RESTARTBLOCK:
			regs->regs[0] = regs->orig_x0;
			regs->pc = restart_addr;
			break;
		}
	}

	if (get_signal(&ksig)) {
		/*
		 * Depending on the signal settings, we may need to revert the
		 * decision to restart the system call, but skip this if a
		 * debugger has chosen to restart at a different PC.
		 */
		if (regs->pc == restart_addr &&
		    (retval == -ERESTARTNOHAND ||
		     retval == -ERESTART_RESTARTBLOCK ||
		     (retval == -ERESTARTSYS))) {
			regs->regs[0] = -EINTR;
			regs->pc = continue_addr;
		}

		handle_signal(&ksig, regs);
		return;
	}

	/*
	 * Handle restarting a different system call. As above, if a debugger
	 * has chosen to restart at a different PC, ignore the restart.
	 */
	if (syscall && regs->pc == restart_addr) {
		if (retval == -ERESTART_RESTARTBLOCK)
			setup_restart_syscall(regs);
	}
}

asmlinkage void do_notify_resume(struct pt_regs *regs,
				 unsigned long thread_flags)
{
	do {
		/* Check valid user FS if needed */
		addr_limit_user_check();

		if (thread_flags & _TIF_NEED_RESCHED) {
			/* Unmask Debug and SError for the next task */
			local_daif_restore(DAIF_PROCCTX_NOIRQ);

			schedule();
		} else {
			local_daif_restore(DAIF_PROCCTX);

			if (thread_flags & _TIF_SIGPENDING)
				do_signal(regs);
		}

		local_daif_mask();
		thread_flags = READ_ONCE(current_thread_info()->flags);
	} while (thread_flags & _TIF_WORK_MASK);
}
