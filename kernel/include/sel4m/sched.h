#ifndef __SEL4M_SCHED_H_
#define __SEL4M_SCHED_H_

#include <base/types.h>

#include <sel4m/mm_types.h>
#include <sel4m/thread.h>
#include <sel4m/sched/sched.h>
#include <sel4m/object/pid.h>
#include <sel4m/object/notifier.h>

#include <uapi/sel4m/object/cap_types.h>

#include <asm/thread_info.h>
#include <asm/processor.h>

/* Attach to any functions which should be ignored in wchan output. */
#define __sched		__attribute__((__section__(".sched.text")))

/* Linker adds these: start and end of __sched functions */
extern char __sched_text_start[], __sched_text_end[];

/* Is this address in the __sched functions? */
extern int in_sched_functions(unsigned long addr);

#define	MAX_SCHEDULE_TIMEOUT	LONG_MAX
extern s64 schedule_timeout(s64 timeout);
extern s64 schedule_timeout_interruptible(s64 timeout);
extern s64 schedule_timeout_uninterruptible(s64 timeout);
asmlinkage void schedule(void);
asmlinkage void preempt_schedule(void);

extern int wake_up_state(struct task_struct * tsk, unsigned int state);
extern int wake_up_process(struct task_struct * tsk);
extern void wake_up_new_task(struct task_struct * tsk,
						unsigned long clone_flags);
extern void kick_process(struct task_struct *tsk);

extern void sched_fork(struct task_struct *p, int clone_flags);

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

#define TASK_NEW		128

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
	/* A live task holds one reference: */
	atomic_t			stack_refcount;

	atomic_t usage;

	unsigned int	flags; /* Per process flags */

	struct pid_struct	pid;

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

	cap_table_t cap_table;

	struct notifier_struct notifier;

    struct mm_struct *mm;

	void __user *cap_ipcptr;

	int exit_state;
	int exit_code, exit_signal;
	int pdeath_signal;  /*  The signal sent when the parent dies  */

	/* Protection of the PI data structures: */
	spinlock_t pi_lock;

	/* Recipient of SIGCHLD, wait4() reports: */
	struct task_struct		*parent;

	struct list_head		children;
	struct list_head		children_list;
	struct list_head		children_exit;

	/* CPU-specific state of this task: */
	struct thread_struct		thread;
};

/*
 * Per process flags
 */
#define PF_IDLE				0x00000001
#define PF_ROOTSERVICE		0x00000002
#define PF_THREAD			0x00000004
#define PF_EXITING			0x00000008

static inline pid_t task_pid_nr(struct task_struct *tsk)
{
	return tsk->pid.pid;
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

static inline unsigned long *end_of_stack(const struct task_struct *task)
{
	return task->stack;
}

static inline void *try_get_task_stack(struct task_struct *tsk)
{
	return atomic_inc_not_zero(&tsk->stack_refcount) ?
		task_stack_page(tsk) : NULL;
}

extern void kfree(const void *);
static inline void put_task_stack(struct task_struct *tsk)
{
	if (atomic_dec_and_test(&tsk->stack_refcount))
		kfree(tsk->stack);
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

static inline void __current_set_polling(void)
{
	set_thread_flag(TIF_POLLING_NRFLAG);
}

static inline void __current_clr_polling(void)
{
	clear_thread_flag(TIF_POLLING_NRFLAG);
}

static inline bool current_clr_polling_and_test(void)
{
	__current_clr_polling();

	/*
	 * Polling state must be visible before we test NEED_RESCHED,
	 * paired by resched_task()
	 */
	smp_mb();

	return unlikely(tif_need_resched());
}

#ifndef tsk_is_polling
#define tsk_is_polling(t) test_tsk_thread_flag(t, TIF_POLLING_NRFLAG)
#endif

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
	if (atomic_dec_and_test(&t->usage))
		BUG();
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

extern cpumask_t nohz_cpu_mask;

/*
 * For kernel-internal use: high-speed (but slightly incorrect) per-cpu
 * clock constructed from sched_clock():
 */
extern unsigned long long cpu_clock(int cpu);
extern unsigned long long
task_sched_runtime(struct task_struct *task);

extern unsigned long weighted_cpuload(const int cpu);

extern void set_task_cpu(struct task_struct *p, unsigned int cpu);

extern void wait_task_inactive(struct task_struct * p);

extern unsigned long nr_running(void);
extern unsigned long nr_uninterruptible(void);
extern unsigned long long nr_context_switches(void);
extern unsigned long nr_iowait(void);
extern unsigned long nr_active(void);

extern void sched_exec(void);

extern int select_nohz_load_balancer(int cpu);
extern void partition_sched_domains(int ndoms_new, cpumask_t *doms_new);

extern long sched_setaffinity(pid_t pid, cpumask_t new_mask);
extern long sched_getaffinity(pid_t pid, cpumask_t *mask);

extern void set_user_nice(struct task_struct *p, long nice);
extern int task_prio(const struct task_struct *p);
extern int task_nice(const struct task_struct *p);
extern int task_curr(const struct task_struct *p);
extern int sched_setscheduler(struct task_struct *, int, struct sched_param *);

asmlinkage long sys_sched_yield(void);
asmlinkage long sys_sched_get_priority_max(int policy);
asmlinkage long sys_sched_get_priority_min(int policy);

extern void yield(void);

extern void io_schedule(void);
extern long io_schedule_timeout(long timeout);

extern void show_task(struct task_struct *p);

#endif /* !__SEL4M_SCHED_H_ */
