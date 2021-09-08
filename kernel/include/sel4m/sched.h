#ifndef __SEL4M_SCHED_H_
#define __SEL4M_SCHED_H_

#include <base/types.h>

#include <sel4m/mm_types.h>
#include <sel4m/thread.h>
#include <sel4m/sched/sched.h>

#include <asm/thread_info.h>
#include <asm/processor.h>

/* Attach to any functions which should be ignored in wchan output. */
#define __sched		__attribute__((__section__(".sched.text")))

/* Linker adds these: start and end of __sched functions */
extern char __sched_text_start[], __sched_text_end[];

#define	MAX_SCHEDULE_TIMEOUT	LONG_MAX
extern s64 schedule_timeout(s64 timeout);
extern s64 schedule_timeout_interruptible(s64 timeout);
extern s64 schedule_timeout_uninterruptible(s64 timeout);
asmlinkage void schedule(void);

extern int wake_up_state(struct task_struct * tsk, unsigned int state);
extern int wake_up_process(struct task_struct * tsk);
extern void wake_up_new_task(struct task_struct * tsk,
						unsigned long clone_flags);
extern void kick_process(struct task_struct *tsk);

/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5

struct sched_param {
	int sched_priority;
};

extern rwlock_t tasklist_lock;

/*
 * Task state bitmask. NOTE! These bits are also
 * encoded in fs/proc/array.c: get_task_state().
 *
 * We have two separate sets of flags: task->state
 * is about runnability, while task->exit_state are
 * about the task exiting. Confusing, but this way
 * modifying one set can't modify the other one by
 * mistake.
 */
#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_STOPPED		4
#define TASK_TRACED		8
/* in tsk->exit_state */
#define EXIT_ZOMBIE		16
#define EXIT_DEAD		32
/* in tsk->state again */
#define TASK_DEAD		64

#define __set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); } while (0)
#define set_task_state(tsk, state_value)		\
	set_mb((tsk)->state, (state_value))

/*
 * set_current_state() includes a barrier so that the write of current->state
 * is correctly serialised wrt the caller's subsequent test of whether to
 * actually sleep:
 *
 *	set_current_state(TASK_UNINTERRUPTIBLE);
 *	if (do_i_need_to_sleep())
 *		schedule();
 *
 * If the caller does not need such serialisation then use __set_current_state()
 */
#define __set_current_state(state_value)			\
	do { current->state = (state_value); } while (0)
#define set_current_state(state_value)		\
	set_mb(current->state, (state_value))

/* Task command name length */
#define TASK_COMM_LEN 16

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

	atomic_t usage;

    pid_t				pid;

	int prio, static_prio, normal_prio;
	struct list_head run_list;
	const struct sched_class *sched_class;
	struct sched_entity se;

	unsigned int policy;
	cpumask_t cpus_allowed;
	unsigned int time_slice;

	unsigned int rt_priority;

	unsigned long nvcsw, nivcsw; /* context switch counts */

	char comm[TASK_COMM_LEN];

	int				oncpu;
	int 			cpu;

    struct mm_struct *mm;

	int exit_state;
	int exit_code, exit_signal;
	int pdeath_signal;  /*  The signal sent when the parent dies  */

	/* Protection of the PI data structures: */
	spinlock_t pi_lock;

	/* Real parent process: */
	struct task_struct		*real_parent;

	/* Recipient of SIGCHLD, wait4() reports: */
	struct task_struct		*parent;

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

/*
 * When accessing the stack of a non-current task that might exit, use
 * try_get_task_stack() instead.  task_stack_page will return a pointer
 * that could get freed out from under you.
 */
static inline void *task_stack_page(const struct task_struct *tsk)
{
	return tsk->stack;
}

/* set thread flags in other task's structures
 * - see asm/thread_info.h for TIF_xxxx flags available
 */
static inline void set_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	set_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline void clear_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	clear_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline int test_and_set_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	return test_and_set_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline int test_and_clear_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	return test_and_clear_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline int test_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	return test_ti_thread_flag(task_thread_info(tsk), flag);
}

static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	set_tsk_thread_flag(tsk, TIF_NEED_RESCHED);
}

static inline void clear_tsk_need_resched(struct task_struct *tsk)
{
	clear_tsk_thread_flag(tsk, TIF_NEED_RESCHED);
}

static inline int signal_pending(struct task_struct *p)
{
	return unlikely(test_tsk_thread_flag(p,TIF_SIGPENDING));
}

static inline int need_resched(void)
{
	return unlikely(test_thread_flag(TIF_NEED_RESCHED));
}

extern struct task_struct init_task;

static inline int idle_cpu(int cpu)
{
	return cpu_curr(cpu) == cpu_rq(cpu)->idle;
}

static inline struct task_struct *idle_task(int cpu)
{
	return cpu_rq(cpu)->idle;
}

#define get_task_struct(tsk) do { atomic_inc(&(tsk)->usage); } while(0)

static inline void put_task_struct(struct task_struct *t)
{
	/* TODO */
}

static inline cpumask_t cpuset_cpus_allowed(struct task_struct *p)
{
	return __cpu_possible_mask;
}
extern int set_cpus_allowed(struct task_struct *p, cpumask_t new_mask);

/*
 * Values used for system_state. Ordering of the states must not be changed
 * as code checks for <, <=, >, >= STATE.
 */
extern enum system_states {
	SYSTEM_BOOTING,
	SYSTEM_SCHEDULING,
	SYSTEM_RUNNING,
	SYSTEM_HALT,
	SYSTEM_POWER_OFF,
	SYSTEM_RESTART,
	SYSTEM_SUSPEND,
} system_state;
extern enum system_states system_state;

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

extern void scheduler_tick(void);

extern void sched_init(void);
extern void sched_init_smp(void);
extern void init_idle(struct task_struct *idle, int cpu);
extern void init_idle_task(struct task_struct *idle);

#endif /* !__SEL4M_SCHED_H_ */
