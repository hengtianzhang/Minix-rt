/*
 *  kernel/sched.c
 *
 *  Kernel scheduler and related syscalls
 *
 *  Copyright (C) 1991-2002  Linus Torvalds
 *
 *  1996-12-23  Modified by Dave Grothe to fix bugs in semaphores and
 *		make semaphores SMP safe
 *  1998-11-19	Implemented schedule_timeout() and related stuff
 *		by Andrea Arcangeli
 *  2002-01-04	New ultra-scalable O(1) scheduler by Ingo Molnar:
 *		hybrid priority-list and round-robin design with
 *		an array-switch method of distributing timeslices
 *		and per-CPU runqueues.  Cleanups and useful suggestions
 *		by Davide Libenzi, preemptible kernel bits by Robert Love.
 *  2003-09-03	Interactivity tuning by Con Kolivas.
 *  2004-04-02	Scheduler domains code by Nick Piggin
 *  2007-04-15  Work begun on replacing all interactivity tuning with a
 *              fair scheduling design by Con Kolivas.
 *  2007-05-05  Load balancing (smp-nice) and other improvements
 *              by Peter Williams
 *  2007-05-06  Interactivity improvements to CFS by Mike Galbraith
 *  2007-07-01  Group scheduling enhancements by Srivatsa Vaddagiri
 */
#include <base/compiler.h>
#include <base/cache.h>

#include <sel4m/reciprocal_div.h>
#include <sel4m/sched.h>
#include <sel4m/mutex.h>
#include <sel4m/jiffies.h>

/*
 * Divide a load by a sched group cpu_power : (load / sg->__cpu_power)
 * Since cpu_power is a 'constant', we can use a reciprocal divide.
 */
static inline u32 sg_div_cpu_power(const struct sched_group *sg, u32 load)
{
	return reciprocal_divide(load, sg->reciprocal_cpu_power);
}

/*
 * Each time a sched group cpu_power is changed,
 * we must compute its reciprocal value
 */
static inline void sg_inc_cpu_power(struct sched_group *sg, u32 val)
{
	sg->__cpu_power += val;
	sg->reciprocal_cpu_power = reciprocal_value(sg->__cpu_power);
}

struct rq runqueues[CONFIG_NR_CPUS] __cacheline_aligned_in_smp;

static DEFINE_MUTEX(sched_hotcpu_mutex);

static inline void check_preempt_curr(struct rq *rq, struct task_struct *p)
{
	rq->curr->sched_class->check_preempt_curr(rq, p);
}

static inline int cpu_of(struct rq *rq)
{
	return rq->cpu;
}

/*
 * Update the per-runqueue clock, as finegrained as the platform can give
 * us, but without assuming monotonicity, etc.:
 */
void __update_rq_clock(struct rq *rq)
{
	u64 prev_raw = rq->prev_clock_raw;
	u64 now = sched_clock();
	s64 delta = now - prev_raw;
	u64 clock = rq->clock;

	/*
	 * Protect against sched_clock() occasionally going backwards:
	 */
	if (unlikely(delta < 0)) {
		clock++;
		rq->clock_warps++;
	} else {
		/*
		 * Catch too large forward jumps too:
		 */
		if (unlikely(clock + delta > rq->tick_timestamp + TICK_NSEC)) {
			if (clock < rq->tick_timestamp + TICK_NSEC)
				clock = rq->tick_timestamp + TICK_NSEC;
			else
				clock++;
			rq->clock_overflows++;
		} else {
			if (unlikely(delta > rq->clock_max_delta))
				rq->clock_max_delta = delta;
			clock += delta;
		}
	}

	rq->prev_clock_raw = now;
	rq->clock = clock;
}

void update_rq_clock(struct rq *rq)
{
	if (likely(smp_processor_id() == cpu_of(rq)))
		__update_rq_clock(rq);
}

/*
 * Number of tasks to iterate in a single balance run.
 * Limited because this is done with IRQs disabled.
 */
const_debug unsigned int sysctl_sched_nr_migrate = 32;

/*
 * For kernel-internal use: high-speed (but slightly incorrect) per-cpu
 * clock constructed from sched_clock():
 */
unsigned long long cpu_clock(int cpu)
{
	unsigned long long now;
	u64 flags;
	struct rq *rq;

	local_irq_save(flags);
	rq = cpu_rq(cpu);
	/*
	 * Only call sched_clock() if the scheduler has already been
	 * initialized (some code might call cpu_clock() very early):
	 */
	if (rq->idle)
		update_rq_clock(rq);
	now = rq->clock;
	local_irq_restore(flags);

	return now;
}

#ifndef prepare_arch_switch
#define prepare_arch_switch(next)	do { } while (0)
#endif
#ifndef finish_arch_switch
#define finish_arch_switch(prev)	do { } while (0)
#endif

static inline int task_current(struct rq *rq, struct task_struct *p)
{
	return rq->curr == p;
}

#ifndef __ARCH_WANT_UNLOCKED_CTXSW
static inline int task_running(struct rq *rq, struct task_struct *p)
{
	return task_current(rq, p);
}

static inline void prepare_lock_switch(struct rq *rq, struct task_struct *next)
{
}

static inline void finish_lock_switch(struct rq *rq, struct task_struct *prev)
{
	/*
	 * If we are tracking spinlock dependencies then we have to
	 * fix up the runqueue lock - which gets 'carried over' from
	 * prev into current:
	 */
	spin_acquire(&rq->lock.dep_map, 0, 0, _THIS_IP_);

	spin_unlock_irq(&rq->lock);
}

#else /* __ARCH_WANT_UNLOCKED_CTXSW */
static inline int task_running(struct rq *rq, struct task_struct *p)
{
	return p->oncpu;
}

static inline void prepare_lock_switch(struct rq *rq, struct task_struct *next)
{
	/*
	 * We can optimise this out completely for !SMP, because the
	 * SMP rebalancing from interrupt is the only thing that cares
	 * here.
	 */
	next->oncpu = 1;

#ifdef __ARCH_WANT_INTERRUPTS_ON_CTXSW
	spin_unlock_irq(&rq->lock);
#else
	spin_unlock(&rq->lock);
#endif
}

static inline void finish_lock_switch(struct rq *rq, struct task_struct *prev)
{
#ifdef CONFIG_SMP
	/*
	 * After ->oncpu is cleared, the task can be moved to a different CPU.
	 * We must ensure this doesn't happen until the switch is completely
	 * finished.
	 */
	smp_wmb();
	prev->oncpu = 0;
#endif
#ifndef __ARCH_WANT_INTERRUPTS_ON_CTXSW
	local_irq_enable();
#endif
}
#endif /* __ARCH_WANT_UNLOCKED_CTXSW */

/*
 * __task_rq_lock - lock the runqueue a given task resides on.
 * Must be called interrupts disabled.
 */
static inline struct rq *__task_rq_lock(struct task_struct *p)
	__acquires(rq->lock)
{
	for (;;) {
		struct rq *rq = task_rq(p);
		spin_lock(&rq->lock);
		if (likely(rq == task_rq(p)))
			return rq;
		spin_unlock(&rq->lock);
	}
}

/*
 * task_rq_lock - lock the runqueue a given task resides on and disable
 * interrupts. Note the ordering: we can safely lookup the task_rq without
 * explicitly disabling preemption.
 */
static struct rq *task_rq_lock(struct task_struct *p, u64 *flags)
	__acquires(rq->lock)
{
	struct rq *rq;

	for (;;) {
		local_irq_save(*flags);
		rq = task_rq(p);
		spin_lock(&rq->lock);
		if (likely(rq == task_rq(p)))
			return rq;
		spin_unlock_irqrestore(&rq->lock, *flags);
	}
}

static void __task_rq_unlock(struct rq *rq)
	__releases(rq->lock)
{
	spin_unlock(&rq->lock);
}

static inline void task_rq_unlock(struct rq *rq, unsigned long *flags)
	__releases(rq->lock)
{
	spin_unlock_irqrestore(&rq->lock, *flags);
}

/*
 * this_rq_lock - lock this runqueue and disable interrupts.
 */
static struct rq *this_rq_lock(void)
	__acquires(rq->lock)
{
	struct rq *rq;

	local_irq_disable();
	rq = this_rq();
	spin_lock(&rq->lock);

	return rq;
}

/*
 * We are going deep-idle (irqs are disabled):
 */
void sched_clock_idle_sleep_event(void)
{
	struct rq *rq = cpu_rq(smp_processor_id());

	spin_lock(&rq->lock);
	__update_rq_clock(rq);
	spin_unlock(&rq->lock);
	rq->clock_deep_idle_events++;
}

/*
 * We just idled delta nanoseconds (called with irqs disabled):
 */
void sched_clock_idle_wakeup_event(u64 delta_ns)
{
	struct rq *rq = cpu_rq(smp_processor_id());
	u64 now = sched_clock();

	rq->idle_clock += delta_ns;
	/*
	 * Override the previous timestamp and ignore all
	 * sched_clock() deltas that occured while we idled,
	 * and use the PM-provided delta_ns to advance the
	 * rq clock:
	 */
	spin_lock(&rq->lock);
	rq->prev_clock_raw = now;
	rq->clock += delta_ns;
	spin_unlock(&rq->lock);
}

#ifndef tsk_is_polling
#define tsk_is_polling(t) test_tsk_thread_flag(t, TIF_POLLING_NRFLAG)
#endif

void resched_task(struct task_struct *p)
{

}

unsigned long
balance_tasks(struct rq *this_rq, int this_cpu, struct rq *busiest,
	      unsigned long max_load_move, struct sched_domain *sd,
	      enum cpu_idle_type idle, int *all_pinned,
	      int *this_best_prio, struct rq_iterator *iterator)
{
	return 0;
}

int
iter_move_one_task(struct rq *this_rq, int this_cpu, struct rq *busiest,
		   struct sched_domain *sd, enum cpu_idle_type idle,
		   struct rq_iterator *iterator)
{
	return 0;
}

