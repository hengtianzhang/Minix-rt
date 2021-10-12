#include <base/linkage.h>
#include <base/common.h>

#include <minix_rt/sched.h>
#include <minix_rt/syscalls.h>

#include <asm/daifflags.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/esr.h>

#undef __UAPI_ASM_SYSCALLS_H_
#undef __SYSCALL
#define __SYSCALL(nr, sym)	asmlinkage long __arm64_##sym(const struct pt_regs *);
#undef __UAPI_ASM_SYSCALLS_H_
#include <uapi/asm/unistd.h>

#undef __SYSCALL
#define __SYSCALL(nr, sym)	[(-nr) - 1] = (syscall_fn_t)__arm64_##sym,
const syscall_fn_t sys_call_table[(-__NR_syscalls) - 1] = {
#undef __UAPI_ASM_SYSCALLS_H_
#include <uapi/asm/unistd.h>
};

static inline bool has_syscall_work(unsigned long flags)
{
	return unlikely(flags & _TIF_SYSCALL_WORK);
}

static long __invoke_syscall(struct pt_regs *regs, syscall_fn_t syscall_fn)
{
	return syscall_fn(regs);
}

static void invoke_syscall(struct pt_regs *regs, int scno,
				int sc_nr,
				const syscall_fn_t syscall_table[])
{
	long ret;

	if ((sc_nr < scno) && (scno < 0)) {
		syscall_fn_t syscall_fn;
		syscall_fn = syscall_table[(-scno) - 1];
		ret = __invoke_syscall(regs, syscall_fn);
	} else {
		printf("Now, TODO syscall %d\n", scno);
		ret = -ENOSYS;
	}

	regs->regs[0] = ret;
}

static void el0_svc_common(struct pt_regs *regs, int scno, int sc_nr,
				const syscall_fn_t syscall_table[])
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

    invoke_syscall(regs, scno, sc_nr, syscall_table);
}

asmlinkage void el0_svc_handler(struct pt_regs *regs)
{
    el0_svc_common(regs, regs->regs[8], __NR_syscalls, sys_call_table);
}
