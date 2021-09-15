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
#ifndef __ASM_STACKTRACE_H_
#define __ASM_STACKTRACE_H_

struct stackframe {
	unsigned long fp;
	unsigned long pc;
};

enum stack_type {
	STACK_TYPE_UNKNOWN,
	STACK_TYPE_TASK,
	STACK_TYPE_IRQ,
	STACK_TYPE_OVERFLOW,
	STACK_TYPE_SDEI_NORMAL,
	STACK_TYPE_SDEI_CRITICAL,
};

struct stack_info {
	unsigned long low;
	unsigned long high;
	enum stack_type type;
};

struct pt_regs;
struct task_struct;

extern int unwind_frame(struct task_struct *tsk, struct stackframe *frame);
extern void dump_backtrace(struct pt_regs *regs, struct task_struct *tsk);

static inline bool on_task_stack(struct task_struct *tsk, unsigned long sp,
				struct stack_info *info)
{
	unsigned long low = (unsigned long)task_stack_page(tsk);
	unsigned long high = low + THREAD_SIZE;

	if (sp < low || sp >= high)
		return false;

	if (info) {
		info->low = low;
		info->high = high;
		info->type = STACK_TYPE_TASK;
	}

	return true;
}

extern u64 *irq_stack_ptr[CONFIG_NR_CPUS];

static inline bool on_irq_stack(unsigned long sp,
				struct stack_info *info)
{
	unsigned long low = (unsigned long)irq_stack_ptr[smp_processor_id()];
	unsigned long high = low + IRQ_STACK_SIZE;

	if (!low)
		return false;

	if (sp < low || sp >= high)
		return false;

	if (info) {
		info->low = low;
		info->high = high;
		info->type = STACK_TYPE_IRQ;
	}

	return true;
}

static inline bool on_overflow_stack(unsigned long sp,
			struct stack_info *info) { return false; }

/*
 * We can only safely access per-cpu stacks from current in a non-preemptible
 * context.
 */
static inline bool on_accessible_stack(struct task_struct *tsk,
					unsigned long sp,
					struct stack_info *info)
{
	if (on_task_stack(tsk, sp, info))
		return true;
	if (tsk != current || preemptible())
		return false;
	if (on_irq_stack(sp, info))
		return true;
	if (on_overflow_stack(sp, info))
		return true;

	return false;
}

#endif /* !__ASM_STACKTRACE_H_ */
