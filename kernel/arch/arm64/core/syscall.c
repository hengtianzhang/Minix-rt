#include <base/linkage.h>
#include <base/common.h>

#include <sel4m/sched.h>
#include <sel4m/object/tcb.h>
#include <sel4m/syscalls.h>

#include <asm/daifflags.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/esr.h>

static inline bool has_syscall_work(unsigned long flags)
{
	return unlikely(flags & _TIF_SYSCALL_WORK);
}

static void invoke_syscall(struct pt_regs *regs, int scno)
{
	long ret = 0;

    switch (scno) {
        case __NR_debug_printf:
        ret = sys_debug_printf((void *)regs->regs[0], regs->regs[1]);
        break;
    }

	regs->regs[0] = ret;
}

static void el0_svc_common(struct pt_regs *regs, int scno)
{
    unsigned long flags = current_thread_info()->flags;

	regs->orig_x0 = regs->regs[0];
	regs->syscallno = scno;

    local_daif_restore(DAIF_PROCCTX);

	if (has_syscall_work(flags)) {
		/* set default errno for user-issued syscall(-1) */
		if (scno == NO_SYSCALL)
			regs->regs[0] = -ENOSYS;
	}

    invoke_syscall(regs, scno);
}

asmlinkage void el0_svc_handler(struct pt_regs *regs)
{
    el0_svc_common(regs, regs->regs[8]);
}
