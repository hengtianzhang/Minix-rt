#ifndef __SEL4M_SCHED_H_
#define __SEL4M_SCHED_H_

#include <base/types.h>

#include <sel4m/mm_types.h>

#include <asm/thread_info.h>
#include <asm/processor.h>

struct task_struct {
	/*
	 * For reasons of header soup (see current_thread_info()), this
	 * must be the first element of task_struct.
	 */
	struct thread_info		thread_info;

	/* -1 unrunnable, 0 runnable, >0 stopped: */
	volatile long			state;

#ifdef CONFIG_STACKPROTECTOR
	/* Canary value for the -fstack-protector GCC feature: */
	u64			stack_canary;
#endif
	void				*stack;

    pid_t				pid;

    struct mm_struct *mm;

	/* CPU-specific state of this task: */
	struct thread_struct		thread;
};

static inline pid_t task_pid_nr(struct task_struct *tsk)
{
	return tsk->pid;
}

extern struct task_struct init_task;

#endif /* !__SEL4M_SCHED_H_ */
