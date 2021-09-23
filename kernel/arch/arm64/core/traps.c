/*
 * Based on arch/arm/kernel/traps.c
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
#include <base/common.h>
#include <base/linkage.h>

#include <sel4m/smp.h>
#include <sel4m/sched.h>
#include <sel4m/stat.h>
#include <sel4m/spinlock.h>
#include <sel4m/object/notifier.h>

#include <asm/current.h>
#include <asm/esr.h>
#include <asm/daifflags.h>
#include <asm/stacktrace.h>
#include <asm/system_misc.h>
#include <asm/exception.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <asm/arch_timer.h>

static const char *handler[]= {
	"Synchronous Abort",
	"IRQ",
	"FIQ",
	"Error"
};

int show_unhandled_signals = 0;

static void dump_backtrace_entry(unsigned long where)
{
	printf(" %p\n", (void *)where);
}

static void __dump_instr(const char *lvl, struct pt_regs *regs)
{
	unsigned long addr = regs->pc;
	char str[sizeof("00000000 ") * 5 + 2 + 1], *p = str;
	int i;

	for (i = -4; i < 1; i++) {
		unsigned int val, bad;

		bad = get_user(val, &((u32 *)addr)[i]);

		if (!bad)
			p += sprintf(p, i == 0 ? "(%08x) " : "%08x ", val);
		else {
			p += sprintf(p, "bad PC value");
			break;
		}
	}
	printf("%sCode: %s\n", lvl, str);
}

static void dump_instr(const char *lvl, struct pt_regs *regs)
{
	if (!user_mode(regs)) {
		mm_segment_t fs = get_fs();
		set_fs(KERNEL_DS);
		__dump_instr(lvl, regs);
		set_fs(fs);
	} else {
		__dump_instr(lvl, regs);
	}
}

/*
 * AArch64 PCS assigns the frame pointer to x29.
 *
 * A simple function prologue looks like this:
 * 	sub	sp, sp, #0x10
 *   	stp	x29, x30, [sp]
 *	mov	x29, sp
 *
 * A simple function epilogue looks like this:
 *	mov	sp, x29
 *	ldp	x29, x30, [sp]
 *	add	sp, sp, #0x10
 */
int notrace unwind_frame(struct task_struct *tsk, struct stackframe *frame)
{
	unsigned long fp = frame->fp;

	if (fp & 0xf)
		return -EINVAL;

	if (!tsk)
		tsk = current;

	if (!on_accessible_stack(tsk, fp, NULL))
		return -EINVAL;

	frame->fp = READ_ONCE(*(unsigned long *)(fp));
	frame->pc = READ_ONCE(*(unsigned long *)(fp + 8));

	/*
	 * Frames created upon entry from EL0 have NULL FP and PC values, so
	 * don't bother reporting these. Frames created by __noreturn functions
	 * might have a valid FP even if PC is bogus, so only terminate where
	 * both are NULL.
	 */
	if (!frame->fp && !frame->pc)
		return -EINVAL;

	return 0;
}

void dump_backtrace(struct pt_regs *regs, struct task_struct *tsk)
{
	struct stackframe frame;
	int skip;

	pr_debug("%s(regs = %p tsk = %p)\n", __func__, regs, tsk);

	if (!tsk)
		tsk = current;

	if (!try_get_task_stack(tsk))
		return;

	if (tsk == current) {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.pc = (unsigned long)dump_backtrace;
	} else {
		/*
		 * task blocked in __switch_to
		 */
		frame.fp = thread_saved_fp(tsk);
		frame.pc = thread_saved_pc(tsk);
	}

	skip = !!regs;
	printf("Call trace:\n");
	do {
		/* skip until specified stack frame */
		if (!skip) {
			dump_backtrace_entry(frame.pc);
		} else if (frame.fp == regs->regs[29]) {
			skip = 0;
			/*
			 * Mostly, this is the case where this function is
			 * called in panic/abort. As exception handler's
			 * stack frame does not contain the corresponding pc
			 * at which an exception has taken place, use regs->pc
			 * instead.
			 */
			dump_backtrace_entry(regs->pc);
		}
	} while (!unwind_frame(tsk, &frame));

	put_task_stack(tsk);
}

void show_stack(struct task_struct *tsk, unsigned long *sp)
{
	dump_backtrace(NULL, tsk);
	barrier();
}

#define S_PREEMPT " PREEMPT"
#define S_SMP " SMP"

static int __die(const char *str, int err, struct pt_regs *regs)
{
	struct task_struct *tsk = current;
	static int die_counter;
	int ret = 0;

	printf("Internal error: %s: %x [#%d]" S_PREEMPT S_SMP "\n",
		 str, err, ++die_counter);

// TODO
//	/* trap and error numbers are mostly meaningless on ARM */
//	ret = notify_die(DIE_OOPS, str, regs, err, 0, SIGSEGV);
//	if (ret == NOTIFY_STOP)
//		return ret;

	__show_regs(regs);
	printf("Process %.*s (pid: %d, stack limit = 0x%p)\n",
		 TASK_COMM_LEN, tsk->comm, task_pid_nr(tsk),
		 end_of_stack(tsk));

	if (!user_mode(regs)) {
		dump_backtrace(regs, tsk);
		dump_instr("", regs);
	}

	return ret;
}

static DEFINE_RAW_SPINLOCK(die_lock);

/*
 * This function is protected against re-entrancy.
 */
void die(const char *str, struct pt_regs *regs, int err)
{
	int ret;
	u64 flags;

	raw_spin_lock_irqsave(&die_lock, flags);

	ret = __die(str, err, regs);

	if (in_interrupt())
		hang("Fatal exception in interrupt");

	raw_spin_unlock_irqrestore(&die_lock, flags);

	// TODO
	//if (ret != NOTIFY_STOP)
		do_exit(SIGSEGV);
}

static void arm64_show_signal(int signo, const char *str)
{
	struct task_struct *tsk = current;
	unsigned int esr = tsk->thread.fault_code;
	struct pt_regs *regs = task_pt_regs(tsk);

	printf("%s[%d]: unhandled exception: ", tsk->comm, task_pid_nr(tsk));
	if (esr)
		printf(KERN_CONT "%s, ESR 0x%08x, ", esr_get_class_string(esr), esr);

	printf(KERN_CONT "%s", str);
// TODO
//	print_vma_addr(KERN_CONT " in ", regs->pc);
	printf(KERN_CONT "\n");
	__show_regs(regs);
}

void arm64_force_sig_fault(int signo, int code, void __user *addr,
			   const char *str)
{
	arm64_show_signal(signo, str);
// TODO
//	force_sig_fault(signo, code, addr, current);
	BUG_ON(1);
}

void arm64_force_sig_mceerr(int code, void __user *addr, short lsb,
			    const char *str)
{
	arm64_show_signal(SIGBUS, str);
// TODO
//	force_sig_mceerr(code, addr, lsb, current);
}

void arm64_notify_die(const char *str, struct pt_regs *regs,
		      int signo, int sicode, void __user *addr,
		      int err)
{
	if (user_mode(regs)) {
		WARN_ON(regs != task_pt_regs(current));
		current->thread.fault_address = 0;
		current->thread.fault_code = err;

		arm64_force_sig_fault(signo, sicode, addr, str);
	} else {
		die(str, regs, err);
	}
}

void arm64_skip_faulting_instruction(struct pt_regs *regs, unsigned long size)
{
	regs->pc += size;
// TODO
	/*
	 * If we were single stepping, we want to get the step exception after
	 * we return from the trap.
	 */
//	if (user_mode(regs))
//		user_fastforward_single_step(current);
}

void force_signal_inject(int signal, int code, unsigned long address)
{
	const char *desc;
	struct pt_regs *regs = task_pt_regs(current);

	if (WARN_ON(!user_mode(regs)))
		return;

	switch (signal) {
	case SIGILL:
		desc = "undefined instruction";
		break;
	case SIGSEGV:
		desc = "illegal memory access";
		break;
	default:
		desc = "unknown or unrecoverable error";
		break;
	}

// TODO
	/* Force signals we don't understand to SIGKILL */
//	if (WARN_ON(signal != SIGKILL &&
//		    siginfo_layout(signal, code) != SIL_FAULT)) {
//		signal = SIGKILL;
//	}

	arm64_notify_die(desc, regs, signal, code, (void __user *)address, 0);
}

/*
 * Set up process info to signal segmentation fault - called on access error.
 */
void arm64_notify_segfault(unsigned long addr)
{
// TODO
}

asmlinkage void __exception do_undefinstr(struct pt_regs *regs)
{
	BUG_ON(!user_mode(regs));
	force_signal_inject(SIGILL, ILL_ILLOPC, regs->pc);
}

#define __user_cache_maint(insn, address, res)			\
	if (address >= user_addr_max()) {			\
		res = -EFAULT;					\
	} else {						\
		asm volatile (					\
			"1:	" insn ", %1\n"			\
			"	mov	%w0, #0\n"		\
			"2:\n"					\
			"	.pushsection .fixup,\"ax\"\n"	\
			"	.align	2\n"			\
			"3:	mov	%w0, %w2\n"		\
			"	b	2b\n"			\
			"	.popsection\n"			\
			_ASM_EXTABLE(1b, 3b)			\
			: "=r" (res)				\
			: "r" (address), "i" (-EFAULT));	\
	}

static void user_cache_maint_handler(unsigned int esr, struct pt_regs *regs)
{
	unsigned long address;
	int rt = ESR_ELx_SYS64_ISS_RT(esr);
	int crm = (esr & ESR_ELx_SYS64_ISS_CRM_MASK) >> ESR_ELx_SYS64_ISS_CRM_SHIFT;
	int ret = 0;

	address = untagged_addr(pt_regs_read_reg(regs, rt));

	switch (crm) {
	case ESR_ELx_SYS64_ISS_CRM_DC_CVAU:	/* DC CVAU, gets promoted */
		__user_cache_maint("dc civac", address, ret);
		break;
	case ESR_ELx_SYS64_ISS_CRM_DC_CVAC:	/* DC CVAC, gets promoted */
		__user_cache_maint("dc civac", address, ret);
		break;
	case ESR_ELx_SYS64_ISS_CRM_DC_CVAP:	/* DC CVAP */
		__user_cache_maint("sys 3, c7, c12, 1", address, ret);
		break;
	case ESR_ELx_SYS64_ISS_CRM_DC_CIVAC:	/* DC CIVAC */
		__user_cache_maint("dc civac", address, ret);
		break;
	case ESR_ELx_SYS64_ISS_CRM_IC_IVAU:	/* IC IVAU */
		__user_cache_maint("ic ivau", address, ret);
		break;
	default:
		force_signal_inject(SIGILL, ILL_ILLOPC, regs->pc);
		return;
	}

	if (ret)
		arm64_notify_segfault(address);
	else
		arm64_skip_faulting_instruction(regs, AARCH64_INSN_SIZE);
}

static void cntvct_read_handler(unsigned int esr, struct pt_regs *regs)
{
	int rt = ESR_ELx_SYS64_ISS_RT(esr);

	pt_regs_write_reg(regs, rt, arch_counter_get_cntvct());
	arm64_skip_faulting_instruction(regs, AARCH64_INSN_SIZE);
}

static void cntfrq_read_handler(unsigned int esr, struct pt_regs *regs)
{
	int rt = ESR_ELx_SYS64_ISS_RT(esr);

	pt_regs_write_reg(regs, rt, arch_timer_get_rate());
	arm64_skip_faulting_instruction(regs, AARCH64_INSN_SIZE);
}

static void wfi_handler(unsigned int esr, struct pt_regs *regs)
{
	arm64_skip_faulting_instruction(regs, AARCH64_INSN_SIZE);
}

struct sys64_hook {
	unsigned int esr_mask;
	unsigned int esr_val;
	void (*handler)(unsigned int esr, struct pt_regs *regs);
};

static struct sys64_hook sys64_hooks[] = {
	{
		.esr_mask = ESR_ELx_SYS64_ISS_EL0_CACHE_OP_MASK,
		.esr_val = ESR_ELx_SYS64_ISS_EL0_CACHE_OP_VAL,
		.handler = user_cache_maint_handler,
	},
	{
		/* Trap read access to CNTVCT_EL0 */
		.esr_mask = ESR_ELx_SYS64_ISS_SYS_OP_MASK,
		.esr_val = ESR_ELx_SYS64_ISS_SYS_CNTVCT,
		.handler = cntvct_read_handler,
	},
	{
		/* Trap read access to CNTFRQ_EL0 */
		.esr_mask = ESR_ELx_SYS64_ISS_SYS_OP_MASK,
		.esr_val = ESR_ELx_SYS64_ISS_SYS_CNTFRQ,
		.handler = cntfrq_read_handler,
	},
	{
		/* Trap WFI instructions executed in userspace */
		.esr_mask = ESR_ELx_WFx_MASK,
		.esr_val = ESR_ELx_WFx_WFI_VAL,
		.handler = wfi_handler,
	},
	{},
};

asmlinkage void __exception do_sysinstr(unsigned int esr, struct pt_regs *regs)
{
	struct sys64_hook *hook;

	for (hook = sys64_hooks; hook->handler; hook++)
		if ((hook->esr_mask & esr) == hook->esr_val) {
			hook->handler(esr, regs);
			return;
		}

	/*
	 * New SYS instructions may previously have been undefined at EL0. Fall
	 * back to our usual undefined instruction handler so that we handle
	 * these consistently.
	 */
	do_undefinstr(regs);
}

static const char *esr_class_str[] = {
	[0 ... ESR_ELx_EC_MAX]		= "UNRECOGNIZED EC",
	[ESR_ELx_EC_UNKNOWN]		= "Unknown/Uncategorized",
	[ESR_ELx_EC_WFx]		= "WFI/WFE",
	[ESR_ELx_EC_CP15_32]		= "CP15 MCR/MRC",
	[ESR_ELx_EC_CP15_64]		= "CP15 MCRR/MRRC",
	[ESR_ELx_EC_CP14_MR]		= "CP14 MCR/MRC",
	[ESR_ELx_EC_CP14_LS]		= "CP14 LDC/STC",
	[ESR_ELx_EC_FP_ASIMD]		= "ASIMD",
	[ESR_ELx_EC_CP10_ID]		= "CP10 MRC/VMRS",
	[ESR_ELx_EC_CP14_64]		= "CP14 MCRR/MRRC",
	[ESR_ELx_EC_ILL]		= "PSTATE.IL",
	[ESR_ELx_EC_SVC32]		= "SVC (AArch32)",
	[ESR_ELx_EC_HVC32]		= "HVC (AArch32)",
	[ESR_ELx_EC_SMC32]		= "SMC (AArch32)",
	[ESR_ELx_EC_SVC64]		= "SVC (AArch64)",
	[ESR_ELx_EC_HVC64]		= "HVC (AArch64)",
	[ESR_ELx_EC_SMC64]		= "SMC (AArch64)",
	[ESR_ELx_EC_SYS64]		= "MSR/MRS (AArch64)",
	[ESR_ELx_EC_SVE]		= "SVE",
	[ESR_ELx_EC_IMP_DEF]		= "EL3 IMP DEF",
	[ESR_ELx_EC_IABT_LOW]		= "IABT (lower EL)",
	[ESR_ELx_EC_IABT_CUR]		= "IABT (current EL)",
	[ESR_ELx_EC_PC_ALIGN]		= "PC Alignment",
	[ESR_ELx_EC_DABT_LOW]		= "DABT (lower EL)",
	[ESR_ELx_EC_DABT_CUR]		= "DABT (current EL)",
	[ESR_ELx_EC_SP_ALIGN]		= "SP Alignment",
	[ESR_ELx_EC_FP_EXC32]		= "FP (AArch32)",
	[ESR_ELx_EC_FP_EXC64]		= "FP (AArch64)",
	[ESR_ELx_EC_SERROR]		= "SError",
	[ESR_ELx_EC_BREAKPT_LOW]	= "Breakpoint (lower EL)",
	[ESR_ELx_EC_BREAKPT_CUR]	= "Breakpoint (current EL)",
	[ESR_ELx_EC_SOFTSTP_LOW]	= "Software Step (lower EL)",
	[ESR_ELx_EC_SOFTSTP_CUR]	= "Software Step (current EL)",
	[ESR_ELx_EC_WATCHPT_LOW]	= "Watchpoint (lower EL)",
	[ESR_ELx_EC_WATCHPT_CUR]	= "Watchpoint (current EL)",
	[ESR_ELx_EC_BKPT32]		= "BKPT (AArch32)",
	[ESR_ELx_EC_VECTOR32]		= "Vector catch (AArch32)",
	[ESR_ELx_EC_BRK64]		= "BRK (AArch64)",
};

const char *esr_get_class_string(u32 esr)
{
	return esr_class_str[ESR_ELx_EC(esr)];
}

/*
 * bad_mode handles the impossible case in the exception vector. This is always
 * fatal.
 */
asmlinkage void bad_mode(struct pt_regs *regs, int reason, unsigned int esr)
{
	printf("Bad mode in %s handler detected on CPU%lld, code 0x%08x -- %s\n",
		handler[reason], smp_processor_id(), esr,
		esr_get_class_string(esr));

	local_daif_mask();
	hang("bad mode");
}

/*
 * bad_el0_sync handles unexpected, but potentially recoverable synchronous
 * exceptions taken from EL0. Unlike bad_mode, this returns.
 */
asmlinkage void bad_el0_sync(struct pt_regs *regs, int reason, unsigned int esr)
{
	void __user *pc = (void __user *)regs->pc;

	current->thread.fault_address = 0;
	current->thread.fault_code = esr;

	arm64_force_sig_fault(SIGILL, ILL_ILLOPC, pc,
			      "Bad EL0 synchronous exception");
}

asmlinkage void do_serror(struct pt_regs *regs, unsigned int esr)
{
	nmi_enter();

	nmi_exit();
}

void __pte_error(const char *file, int line, unsigned long val)
{
	printf("%s:%d: bad pte %016lx.\n", file, line, val);
}

void __pmd_error(const char *file, int line, unsigned long val)
{
	printf("%s:%d: bad pmd %016lx.\n", file, line, val);
}

void __pud_error(const char *file, int line, unsigned long val)
{
	printf("%s:%d: bad pud %016lx.\n", file, line, val);
}

void __pgd_error(const char *file, int line, unsigned long val)
{
	printf("%s:%d: bad pgd %016lx.\n", file, line, val);
}

/*
 * Initial handler for AArch64 BRK exceptions
 * This handler only used until debug_traps_init().
 */
int __init early_brk64(unsigned long addr, unsigned int esr,
		struct pt_regs *regs)
{
	return -1;
}
