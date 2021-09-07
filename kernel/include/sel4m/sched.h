#ifndef __SEL4M_SCHED_H_
#define __SEL4M_SCHED_H_

#include <base/types.h>

#include <sel4m/mm_types.h>
#include <sel4m/sched/sched.h>

#include <asm/thread_info.h>
#include <asm/processor.h>

/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5

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

	int prio, static_prio, normal_prio;
	struct list_head run_list;
	const struct sched_class *sched_class;
	struct sched_entity se;

	unsigned int policy;
	cpumask_t cpus_allowed;
	unsigned int time_slice;

	int cpu;

    struct mm_struct *mm;

	/* CPU-specific state of this task: */
	struct thread_struct		thread;
};

static inline pid_t task_pid_nr(struct task_struct *tsk)
{
	return tsk->pid;
}

static inline unsigned int task_cpu(const struct task_struct *tsk)
{
	return tsk->cpu;
}

static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	//set_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

extern struct task_struct init_task;

extern void resched_task(struct task_struct *p);
extern unsigned long
balance_tasks(struct rq *this_rq, int this_cpu, struct rq *busiest,
	      unsigned long max_load_move, struct sched_domain *sd,
	      enum cpu_idle_type idle, int *all_pinned,
	      int *this_best_prio, struct rq_iterator *iterator);
extern int
iter_move_one_task(struct rq *this_rq, int this_cpu, struct rq *busiest,
		   struct sched_domain *sd, enum cpu_idle_type idle,
		   struct rq_iterator *iterator);

extern void update_rq_clock(struct rq *rq);
extern void __update_rq_clock(struct rq *rq);
#endif /* !__SEL4M_SCHED_H_ */
