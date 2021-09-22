#ifndef __SEL4M_SCHED_SCHED_H_
#define __SEL4M_SCHED_SCHED_H_

#include <base/types.h>
#include <base/compiler.h>
#include <base/rbtree.h>
#include <base/bitmap.h>
#include <base/list.h>

#include <sel4m/lockdep.h>
#include <sel4m/spinlock.h>
#include <sel4m/smp.h>
#include <sel4m/cpumask.h>
#include <sel4m/mm_types.h>
#include <sel4m/sched/clock.h>
#include <sel4m/sched/prio.h>

struct task_struct;

#define const_debug static const

/*
 * Debugging: various feature bits
 */
enum {
	SCHED_FEAT_NEW_FAIR_SLEEPERS	= 1,
	SCHED_FEAT_WAKEUP_PREEMPT	= 2,
	SCHED_FEAT_START_DEBIT		= 4,
	SCHED_FEAT_TREE_AVG		= 8,
	SCHED_FEAT_APPROX_AVG		= 16,
};

const_debug unsigned int sysctl_sched_features =
		SCHED_FEAT_NEW_FAIR_SLEEPERS	* 1 |
		SCHED_FEAT_WAKEUP_PREEMPT	* 1 |
		SCHED_FEAT_START_DEBIT		* 1 |
		SCHED_FEAT_TREE_AVG		* 0 |
		SCHED_FEAT_APPROX_AVG		* 0;

#define sched_feat(x) (sysctl_sched_features & SCHED_FEAT_##x)

enum cpu_idle_type {
	CPU_IDLE,
	CPU_NOT_IDLE,
	CPU_NEWLY_IDLE,
	CPU_MAX_IDLE_TYPES
};

/*
 * sched-domains (multiprocessor balancing) declarations:
 */

/*
 * Increase resolution of nice-level calculations:
 */
#define SCHED_LOAD_SHIFT	10
#define SCHED_LOAD_SCALE	(1L << SCHED_LOAD_SHIFT)

#define NICE_0_LOAD		SCHED_LOAD_SCALE
#define NICE_0_SHIFT		SCHED_LOAD_SHIFT

#define SCHED_LOAD_SCALE_FUZZ	SCHED_LOAD_SCALE

#define SD_LOAD_BALANCE		1	/* Do load balancing on this domain. */
#define SD_BALANCE_NEWIDLE	2	/* Balance when about to become idle */
#define SD_BALANCE_EXEC		4	/* Balance on exec */
#define SD_BALANCE_FORK		8	/* Balance on fork, clone */
#define SD_WAKE_IDLE		16	/* Wake to idle CPU on task wakeup */
#define SD_WAKE_AFFINE		32	/* Wake task to waking CPU */
#define SD_WAKE_BALANCE		64	/* Perform balancing at task wakeup */
#define SD_SHARE_CPUPOWER	128	/* Domain members share cpu power */
#define SD_POWERSAVINGS_BALANCE	256	/* Balance for power savings */
#define SD_SHARE_PKG_RESOURCES	512	/* Domain members share cpu pkg resources */
#define SD_SERIALIZE		1024	/* Only a single load balancing instance */

#define test_sd_parent(sd, flag)	((sd->parent &&		\
					 (sd->parent->flags & flag)) ? 1 : 0)

/* Common values for CPUs */
#ifndef SD_CPU_INIT
#define SD_CPU_INIT (struct sched_domain) {		\
	.span			= CPU_MASK_NONE,	\
	.parent			= NULL,			\
	.child			= NULL,			\
	.groups			= NULL,			\
	.min_interval		= 1,			\
	.max_interval		= 4,			\
	.busy_factor		= 64,			\
	.imbalance_pct		= 125,			\
	.cache_nice_tries	= 1,			\
	.busy_idx		= 2,			\
	.idle_idx		= 1,			\
	.newidle_idx		= 2,			\
	.wake_idx		= 1,			\
	.forkexec_idx		= 1,			\
	.flags			= SD_LOAD_BALANCE	\
				| SD_BALANCE_NEWIDLE	\
				| SD_BALANCE_EXEC	\
				| SD_BALANCE_FORK	\
				| SD_WAKE_AFFINE,\
	.last_balance		= jiffies,		\
	.balance_interval	= 1,			\
	.nr_balance_failed	= 0,			\
}
#endif

struct sched_group {
	struct sched_group *next;	/* Must be a circular list */
	cpumask_t cpumask;

	/*
	 * CPU power of this group, SCHED_LOAD_SCALE being max power for a
	 * single CPU. This is read only (except for setup, hotplug CPU).
	 * Note : Never change cpu_power without recompute its reciprocal
	 */
	unsigned int __cpu_power;
	/*
	 * reciprocal value of cpu_power to avoid expensive divides
	 * (see include/linux/reciprocal_div.h)
	 */
	u32 reciprocal_cpu_power;
};

struct sched_domain {
	/* These fields must be setup */
	struct sched_domain *parent;	/* top domain must be null terminated */
	struct sched_domain *child;	/* bottom domain must be null terminated */
	struct sched_group *groups;	/* the balancing groups of the domain */
	cpumask_t span;			/* span of all CPUs in this domain */
	unsigned long min_interval;	/* Minimum balance interval ms */
	unsigned long max_interval;	/* Maximum balance interval ms */
	unsigned int busy_factor;	/* less balancing by factor if busy */
	unsigned int imbalance_pct;	/* No balance until over watermark */
	unsigned int cache_nice_tries;	/* Leave cache hot tasks for # tries */
	unsigned int busy_idx;
	unsigned int idle_idx;
	unsigned int newidle_idx;
	unsigned int wake_idx;
	unsigned int forkexec_idx;
	int flags;			/* See SD_* */

	/* Runtime fields. */
	unsigned long last_balance;	/* init to jiffies. units in jiffies */
	unsigned int balance_interval;	/* initialise to 1. units in ms. */
	unsigned int nr_balance_failed; /* initialise to 0 */
};

struct load_weight {
	unsigned long weight, inv_weight;
};

/*
 * CFS stats for a schedulable entity (task, task-group etc)
 *
 * Current field usage histogram:
 *
 *     4 se->block_start
 *     4 se->run_node
 *     4 se->sleep_start
 *     6 se->load.weight
 */
struct sched_entity {
	struct load_weight	load;		/* for load-balancing */
	struct rb_node		run_node;
	unsigned int		on_rq;

	u64			exec_start;
	u64			sum_exec_runtime;
	u64			vruntime;
	u64			prev_sum_exec_runtime;
};

/* CFS-related fields in a runqueue */
struct cfs_rq {
	struct load_weight load;
	unsigned long nr_running;

	u64	exec_clock;
	u64 min_vruntime;

	struct rb_root tasks_timeline;
	struct rb_node *rb_leftmost;
	struct rb_node *rb_load_balance_curr;
	/* 'curr' points to currently running entity on this cfs_rq.
	 * It is set to NULL otherwise (i.e when none are currently running).
	 */
	struct sched_entity *curr;

	unsigned long nr_spread_over;
};

/*
 * This is the priority-queue data structure of the RT scheduling class:
 */
struct rt_prio_array {
	DECLARE_BITMAP(bitmap, MAX_RT_PRIO+1); /* include 1 bit for delimiter */
	struct list_head queue[MAX_RT_PRIO];
};

/* Real-Time classes' related field in a runqueue: */
struct rt_rq {
	struct rt_prio_array active;
	int rt_load_balance_idx;
	struct list_head *rt_load_balance_head, *rt_load_balance_curr;
};

/*
 * This is the main, per-CPU runqueue data structure.
 *
 * Locking rule: those places that want to lock multiple runqueues
 * (such as the load balancing or the thread migration code), lock
 * acquire operations must be ordered by ascending &runqueue.
 */
struct rq {
	/* runqueue lock: */
	spinlock_t lock;

	/*
	 * nr_running and cpu_load should be in the same cacheline because
	 * remote CPUs use both these fields when doing load calculation.
	 */
	unsigned long nr_running;
#define CPU_LOAD_IDX_MAX 5
	unsigned long cpu_load[CPU_LOAD_IDX_MAX];
	unsigned char idle_at_tick;

	unsigned char in_nohz_recently;

	/* capture load from *all* tasks on this cpu: */
	struct load_weight load;
	unsigned long nr_load_updates;
	u64 nr_switches;

	struct cfs_rq cfs;
	struct rt_rq rt;

	/*
	 * This is part of a global counter where only the total sum
	 * over all CPUs matters. A task can increase this counter on
	 * one CPU and if it got migrated afterwards it may decrease
	 * it on another CPU. Always updated under the runqueue lock:
	 */
	unsigned long nr_uninterruptible;

	struct task_struct *curr, *idle;
	u64 next_balance;
	struct mm_struct *prev_mm;

	u64 clock, prev_clock_raw;
	s64 clock_max_delta;

	unsigned int clock_warps, clock_overflows;
	u64 idle_clock;
	unsigned int clock_deep_idle_events;
	u64 tick_timestamp;

	atomic_t nr_iowait;

	struct sched_domain *sd;

	/* For active balancing */
	int active_balance;
	int push_cpu;
	/* cpu of this runqueue: */
	int cpu;

	struct task_struct *migration_thread;
	struct list_head migration_queue;

	struct lock_class_key rq_lock_key;
};


struct sched_class {
	const struct sched_class *next;

	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int wakeup);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int sleep);
	void (*yield_task) (struct rq *rq);

	void (*check_preempt_curr) (struct rq *rq, struct task_struct *p);

	struct task_struct * (*pick_next_task) (struct rq *rq);
	void (*put_prev_task) (struct rq *rq, struct task_struct *p);

	unsigned long (*load_balance) (struct rq *this_rq, int this_cpu,
			struct rq *busiest, unsigned long max_load_move,
			struct sched_domain *sd, enum cpu_idle_type idle,
			int *all_pinned, int *this_best_prio);

	int (*move_one_task) (struct rq *this_rq, int this_cpu,
			      struct rq *busiest, struct sched_domain *sd,
			      enum cpu_idle_type idle);

	void (*set_curr_task) (struct rq *rq);
	void (*task_tick) (struct rq *rq, struct task_struct *p);
	void (*task_new) (struct rq *rq, struct task_struct *p);
};

extern const struct sched_class rt_sched_class;
extern const struct sched_class fair_sched_class;
extern const struct sched_class idle_sched_class;

static inline void update_load_add(struct load_weight *lw, unsigned long inc)
{
	lw->weight += inc;
}

static inline void update_load_sub(struct load_weight *lw, unsigned long dec)
{
	lw->weight -= dec;
}

/*
 * To aid in avoiding the subversion of "niceness" due to uneven distribution
 * of tasks with abnormal "nice" values across CPUs the contribution that
 * each task makes to its run queue's load is weighted according to its
 * scheduling class and "nice" value. For SCHED_NORMAL tasks this is just a
 * scaled version of the new time slice allocation that they receive on time
 * slice expiry etc.
 */

#define WEIGHT_IDLEPRIO		2
#define WMULT_IDLEPRIO		(1 << 31)

/*
 * Nice levels are multiplicative, with a gentle 10% change for every
 * nice level changed. I.e. when a CPU-bound task goes from nice 0 to
 * nice 1, it will get ~10% less CPU time than another CPU-bound task
 * that remained on nice 0.
 *
 * The "10% effect" is relative and cumulative: from _any_ nice level,
 * if you go up 1 level, it's -10% CPU usage, if you go down 1 level
 * it's +10% CPU usage. (to achieve that we use a multiplier of 1.25.
 * If a task goes up by ~10% and another task goes down by ~10% then
 * the relative distance between them is ~25%.)
 */
static const int prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};

/*
 * Inverse (2^32/x) values of the prio_to_weight[] array, precalculated.
 *
 * In cases where the weight does not change often, we can use the
 * precalculated inverse to speed up arithmetics by turning divisions
 * into multiplications:
 */
static const u32 prio_to_wmult[40] = {
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};

/*
 * runqueue iterator, to support SMP load-balancing between different
 * scheduling classes, without having to expose their internal data
 * structures to the load-balancing proper:
 */
struct rq_iterator {
	void *arg;
	struct task_struct *(*start)(void *);
	struct task_struct *(*next)(void *);
};

extern struct rq runqueues[CONFIG_NR_CPUS];

#define cpu_rq(cpu) 	(&runqueues[cpu])
#define this_rq() 		(&runqueues[smp_processor_id()])
#define task_rq(p) 		(cpu_rq(task_cpu(p)))
#define cpu_curr(cpu) 	(cpu_rq(cpu)->curr)

/*
 * The domain tree (rq->sd) is protected by RCU's quiescent state transition.
 * See detach_destroy_domains: synchronize_sched for details.
 *
 * The domain tree of any CPU may only be accessed from within
 * preempt-disabled sections.
 */
#define for_each_domain(cpu, __sd) \
	for (__sd = READ_ONCE(cpu_rq(cpu)->sd); __sd; __sd = __sd->parent)

#endif /* !__SEL4M_SCHED_SCHED_H_ */
