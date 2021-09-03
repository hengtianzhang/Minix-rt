#ifndef __SEL4M_SCHED_H_
#define __SEL4M_SCHED_H_

#include <base/types.h>

#include <sel4m/mm_types.h>

struct task_struct {
	/* -1 unrunnable, 0 runnable, >0 stopped: */
	volatile long			state;
    pid_t				pid;

    struct mm_struct *mm;
};

static inline pid_t task_pid_nr(struct task_struct *tsk)
{
	return tsk->pid;
}

extern struct task_struct init_task;

#endif /* !__SEL4M_SCHED_H_ */
