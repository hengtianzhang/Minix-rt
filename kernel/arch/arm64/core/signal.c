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

#include <asm/esr.h>
#include <asm/daifflags.h>

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

			//if (thread_flags & _TIF_UPROBE)
			//	uprobe_notify_resume(regs);

			//if (thread_flags & _TIF_SIGPENDING)
			//	do_signal(regs);

			//if (thread_flags & _TIF_NOTIFY_RESUME) {
			//	clear_thread_flag(TIF_NOTIFY_RESUME);
			//	tracehook_notify_resume(regs);
			//	rseq_handle_notify_resume(NULL, regs);
			//}

			//if (thread_flags & _TIF_FOREIGN_FPSTATE)
			//	fpsimd_restore_current_state();
		}

		local_daif_mask();
		thread_flags = READ_ONCE(current_thread_info()->flags);
	} while (thread_flags & _TIF_WORK_MASK);
}
