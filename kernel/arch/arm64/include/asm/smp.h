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
#ifndef __ASM_SMP_H_
#define __ASM_SMP_H_

/* Values for secondary_data.status */
#define CPU_STUCK_REASON_SHIFT		(8)
#define CPU_BOOT_STATUS_MASK		((ULL(1) << CPU_STUCK_REASON_SHIFT) - 1)

#define CPU_MMU_OFF			(-1)
#define CPU_BOOT_SUCCESS		(0)
/* The cpu invoked ops->cpu_die, synchronise it with cpu_kill */
#define CPU_KILL_ME			(1)
/* The cpu couldn't die gracefully and is looping in the kernel */
#define CPU_STUCK_IN_KERNEL		(2)
/* Fatal system error detected by secondary CPU, crash the system */
#define CPU_PANIC_KERNEL		(3)

#define CPU_STUCK_REASON_52_BIT_VA	(ULL(1) << CPU_STUCK_REASON_SHIFT)
#define CPU_STUCK_REASON_NO_GRAN	(ULL(2) << CPU_STUCK_REASON_SHIFT)

#ifndef __ASSEMBLY__

#include <sel4m/types.h>
#include <sel4m/linkage.h>
#include <sel4m/sched.h>

#include <asm/stack_pointer.h>

static inline void set_my_cpu_offset(u64 off)
{
	asm volatile("msr tpidr_el1, %0"
			:: "r" (off) : "memory");
}

static inline u64 __my_cpu_offset(void)
{
	u64 off;

	/*
	 * We want to allow caching the value, so avoid using volatile and
	 * instead use a fake stack read to hazard against barrier().
	 */
	asm("mrs %0, tpidr_el1"
		: "=r" (off) :
		"Q" (*(const u64 *)current_stack_pointer));

	return off;
}
#define __my_cpu_offset __my_cpu_offset()

#define raw_smp_processor_id() (__my_cpu_offset)

/*
 * Called from the secondary holding pen, this is the secondary CPU entry point.
 */
asmlinkage void secondary_start_kernel(void);

/*
 * Initial data for bringing up a secondary CPU.
 * @stack  - sp for the secondary CPU
 * @status - Result passed back from the secondary CPU to
 *           indicate failure.
 */
struct secondary_data {
	void *stack;
	struct task_struct *task;
	s64 status;
};

extern struct secondary_data secondary_data;

extern s64 __early_cpu_boot_status;
extern void secondary_entry(void);

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_SMP_H_ */
