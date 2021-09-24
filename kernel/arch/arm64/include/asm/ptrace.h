/*
 * Based on arch/arm/include/asm/ptrace.h
 *
 * Copyright (C) 1996-2003 Russell King
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
#ifndef __ASM_PTRACE_H_
#define __ASM_PTRACE_H_

/*
 * PSR bits
 */
#define PSR_MODE_EL0t	0x00000000
#define PSR_MODE_EL1t	0x00000004
#define PSR_MODE_EL1h	0x00000005
#define PSR_MODE_EL2t	0x00000008
#define PSR_MODE_EL2h	0x00000009
#define PSR_MODE_EL3t	0x0000000c
#define PSR_MODE_EL3h	0x0000000d
#define PSR_MODE_MASK	0x0000000f

/* AArch32 CPSR bits */
#define PSR_MODE32_BIT		0x00000010

/* AArch64 SPSR bits */
#define PSR_F_BIT	0x00000040
#define PSR_I_BIT	0x00000080
#define PSR_A_BIT	0x00000100
#define PSR_D_BIT	0x00000200
#define PSR_SSBS_BIT	0x00001000
#define PSR_PAN_BIT	0x00400000
#define PSR_UAO_BIT	0x00800000
#define PSR_V_BIT	0x10000000
#define PSR_C_BIT	0x20000000
#define PSR_Z_BIT	0x40000000
#define PSR_N_BIT	0x80000000

/*
 * Groups of PSR bits
 */
#define PSR_f		0xff000000	/* Flags		*/
#define PSR_s		0x00ff0000	/* Status		*/
#define PSR_x		0x0000ff00	/* Extension		*/
#define PSR_c		0x000000ff	/* Control		*/

/* Current Exception Level values, as contained in CurrentEL */
#define CurrentEL_EL1		(1 << 2)
#define CurrentEL_EL2		(2 << 2)

/*
 * If pt_regs.syscallno == NO_SYSCALL, then the thread is not executing
 * a syscall -- i.e., its most recent entry into the kernel from
 * userspace was not via SVC, or otherwise a tracer cancelled the syscall.
 *
 * This must have the value -1, for ABI compatibility with ptrace etc.
 */
#define NO_SYSCALL (-1)

#define AARCH64_INSN_SIZE 4

#ifndef __ASSEMBLY__

#include <base/common.h>

#include <asm/base/types.h>
/*
 * User structures for general purpose, floating point and debug registers.
 */
struct user_pt_regs {
	__u64		regs[31];
	__u64		sp;
	__u64		pc;
	__u64		pstate;
};

struct user_fpsimd_state {
	__uint128_t	vregs[32];
	__u32		fpsr;
	__u32		fpcr;
	__u32		__reserved[2];
};

/*
 * This struct defines the way the registers are stored on the stack during an
 * exception. Note that sizeof(struct pt_regs) has to be a multiple of 16 (for
 * stack alignment). struct user_pt_regs must form a prefix of struct pt_regs.
 */
struct pt_regs {
	union {
		struct user_pt_regs user_regs;
		struct {
			u64 regs[31];
			u64 sp;
			u64 pc;
			u64 pstate;
		};
	};
	u64 orig_x0;
#ifdef __AARCH64EB__
	u32 unused2;
	s32 syscallno;
#else
	s32 syscallno;
	u32 unused2;
#endif

	u64 orig_addr_limit;
	u64 unused;	// maintain 16 byte alignment
	u64 stackframe[2];
};

static inline bool in_syscall(struct pt_regs const *regs)
{
	return regs->syscallno != NO_SYSCALL;
}

static inline void forget_syscall(struct pt_regs *regs)
{
	regs->syscallno = NO_SYSCALL;
}

#define user_mode(regs)	\
	(((regs)->pstate & PSR_MODE_MASK) == PSR_MODE_EL0t)

#define processor_mode(regs) \
	((regs)->pstate & PSR_MODE_MASK)

#define interrupts_enabled(regs) \
	(!((regs)->pstate & PSR_I_BIT))

#define fast_interrupts_enabled(regs) \
	(!((regs)->pstate & PSR_F_BIT))

/**
 * regs_get_register() - get register value from its offset
 * @regs:	pt_regs from which register value is gotten
 * @offset:	offset of the register.
 *
 * regs_get_register returns the value of a register whose offset from @regs.
 * The @offset is the offset of the register in struct pt_regs.
 * If @offset is bigger than MAX_REG_OFFSET, this returns 0.
 */
static inline u64 regs_get_register(struct pt_regs *regs, unsigned int offset)
{
	u64 val = 0;

	WARN_ON(offset & 7);

	offset >>= 3;
	switch (offset) {
	case 0 ... 30:
		val = regs->regs[offset];
		break;
	case offsetof(struct pt_regs, sp) >> 3:
		val = regs->sp;
		break;
	case offsetof(struct pt_regs, pc) >> 3:
		val = regs->pc;
		break;
	case offsetof(struct pt_regs, pstate) >> 3:
		val = regs->pstate;
		break;
	default:
		val = 0;
	}

	return val;
}

/*
 * Read a register given an architectural register index r.
 * This handles the common case where 31 means XZR, not SP.
 */
static inline unsigned long pt_regs_read_reg(const struct pt_regs *regs, int r)
{
	return (r == 31) ? 0 : regs->regs[r];
}

/*
 * Write a register given an architectural register index r.
 * This handles the common case where 31 means XZR, not SP.
 */
static inline void pt_regs_write_reg(struct pt_regs *regs, int r,
				     unsigned long val)
{
	if (r != 31)
		regs->regs[r] = val;
}

/* Valid only for Kernel mode traps. */
static inline unsigned long kernel_stack_pointer(struct pt_regs *regs)
{
	return regs->sp;
}

static inline unsigned long regs_return_value(struct pt_regs *regs)
{
	return regs->regs[0];
}

/* We must avoid circular header include via sched.h */
struct task_struct;
int valid_user_regs(struct user_pt_regs *regs, struct task_struct *task);

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_PTRACE_H_ */
