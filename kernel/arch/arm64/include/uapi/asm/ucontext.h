/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
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
#ifndef __UAPI_ASM_UCONTEXT_H_
#define __UAPI_ASM_UCONTEXT_H_

#include <base/types.h>

#ifndef __KERNEL__
#include <asm/signal.h>
#else
#include <uapi/asm/signal.h>
#endif /* !__KERNEL__ */

/*
 * Allocation of __reserved[]:
 * (Note: records do not necessarily occur in the order shown here.)
 *
 *	size		description
 *
 *	0x210		fpsimd_context
 *	 0x10		esr_context
 *	0x8a0		sve_context (vl <= 64) (optional)
 *	 0x20		extra_context (optional)
 *	 0x10		terminator (null _aarch64_ctx)
 *
 *	0x510		(reserved for future allocation)
 *
 * New records that can exceed this space need to be opt-in for userspace, so
 * that an expanded signal frame is not generated unexpectedly.  The mechanism
 * for opting in will depend on the extension that generates each new record.
 * The above table documents the maximum set and sizes of records than can be
 * generated when userspace does not opt in for any such extension.
 */

/*
 * Header to be used at the beginning of structures extending the user
 * context. Such structures must be placed after the rt_sigframe on the stack
 * and be 16-byte aligned. The last structure must be a dummy one with the
 * magic and size set to 0.
 */
struct _aarch64_ctx {
	__u32 magic;
	__u32 size;
};

/* ESR_EL1 context */
#define ESR_MAGIC	0x45535201

struct esr_context {
	struct _aarch64_ctx head;
	__u64 esr;
};

#define FPSIMD_MAGIC	0x46508001

struct fpsimd_context {
	struct _aarch64_ctx head;
	__u32 fpsr;
	__u32 fpcr;
	__uint128_t vregs[32];
};

#define EXTRA_MAGIC	0x45585401

struct extra_context {
	struct _aarch64_ctx head;
	__u64 datap; /* 16-byte aligned pointer to extra space cast to __u64 */
	__u32 size; /* size in bytes of the extra space */
	__u32 __reserved[3];
};

typedef struct sigaltstack {
	void __user *ss_sp;
	int ss_flags;
	size_t ss_size;
} stack_t;

/*
 * Signal context structure - contains all info to do with the state
 * before the signal handler was invoked.
 */
struct sigcontext {
	__u64 fault_address;
	/* AArch64 registers */
	__u64 regs[31];
	__u64 sp;
	__u64 pc;
	__u64 pstate;
	/* 4K reserved for FP/SIMD state and future expansion */
	__u8 __reserved[4096] __attribute__((__aligned__(16)));
};

struct ucontext {
	unsigned long	  uc_flags;
	struct ucontext	 *uc_link;
	stack_t		  uc_stack;
	sigset_t	  uc_sigmask;
	/* glibc uses a 1024-bit sigset_t */
	__u8		  __unused[1024 / 8 - sizeof(sigset_t)];
	/* last for future expansion */
	struct sigcontext uc_mcontext;
};

#endif /* !__UAPI_ASM_UCONTEXT_H_ */