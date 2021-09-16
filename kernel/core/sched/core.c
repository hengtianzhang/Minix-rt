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
#include <base/linkage.h>

#include <sel4m/reciprocal_div.h>
#include <sel4m/sched.h>
#include <sel4m/sched/rt.h>
#include <sel4m/sched/fair.h>
#include <sel4m/completion.h>
#include <sel4m/wait.h>
#include <sel4m/cpumask.h>
#include <sel4m/mutex.h>
#include <sel4m/jiffies.h>
#include <sel4m/interrupt.h>
#include <sel4m/irq.h>
#include <sel4m/object/pid.h>
#include <sel4m/sched/idle.h>
#include <sel4m/irq.h>
#include <sel4m/stat.h>

#include <asm-generic/switch_to.h>

#include <asm/system_misc.h>
#include <asm/mmu_context.h>

__cacheline_aligned DEFINE_RWLOCK(tasklist_lock);  /* outer */

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
	/*
	 * After ->oncpu is cleared, the task can be moved to a different CPU.
	 * We must ensure this doesn't happen until the switch is completely
	 * finished.
	 */
	smp_wmb();
	prev->oncpu = 0;
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

static inline void task_rq_unlock(struct rq *rq, u64 *flags)
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

void resched_task(struct task_struct *p)
{
	int cpu;

	assert_spin_locked(&task_rq(p)->lock);

	if (unlikely(test_tsk_thread_flag(p, TIF_NEED_RESCHED)))
		return;

	set_tsk_thread_flag(p, TIF_NEED_RESCHED);

	cpu = task_cpu(p);
	if (cpu == smp_processor_id())
		return;

	/* NEED_RESCHED must be visible before we test polling */
	smp_mb();

	if (!tsk_is_polling(p))
		smp_send_reschedule(cpu);
}

static void resched_cpu(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	u64 flags;

	if (!spin_trylock_irqsave(&rq->lock, flags))
		return;
	resched_task(cpu_curr(cpu));
	spin_unlock_irqrestore(&rq->lock, flags);
}

#define sched_class_highest (&rt_sched_class)

/*
 * Update delta_exec, delta_fair fields for rq.
 *
 * delta_fair clock advances at a rate inversely proportional to
 * total load (rq->load.weight) on the runqueue, while
 * delta_exec advances at the same rate as wall-clock (provided
 * cpu is not idle).
 *
 * delta_exec / delta_fair is a measure of the (smoothened) load on this
 * runqueue over any given interval. This (smoothened) load is used
 * during load balance.
 *
 * This function is called /before/ updating rq->load
 * and when switching tasks.
 */
static inline void inc_load(struct rq *rq, const struct task_struct *p)
{
	update_load_add(&rq->load, p->se.load.weight);
}

static inline void dec_load(struct rq *rq, const struct task_struct *p)
{
	update_load_sub(&rq->load, p->se.load.weight);
}

static void inc_nr_running(struct task_struct *p, struct rq *rq)
{
	rq->nr_running++;
	inc_load(rq, p);
}

static void dec_nr_running(struct task_struct *p, struct rq *rq)
{
	rq->nr_running--;
	dec_load(rq, p);
}

static void set_load_weight(struct task_struct *p)
{
	if (task_has_rt_policy(p)) {
		p->se.load.weight = prio_to_weight[0] * 2;
		p->se.load.inv_weight = prio_to_wmult[0] >> 1;
		return;
	}

	/*
	 * SCHED_IDLE tasks get minimal weight:
	 */
	if (p->policy == SCHED_IDLE) {
		p->se.load.weight = WEIGHT_IDLEPRIO;
		p->se.load.inv_weight = WMULT_IDLEPRIO;
		return;
	}

	p->se.load.weight = prio_to_weight[p->static_prio - MAX_RT_PRIO];
	p->se.load.inv_weight = prio_to_wmult[p->static_prio - MAX_RT_PRIO];
}

static void enqueue_task(struct rq *rq, struct task_struct *p, int wakeup)
{
	p->sched_class->enqueue_task(rq, p, wakeup);
	p->se.on_rq = 1;
}

static void dequeue_task(struct rq *rq, struct task_struct *p, int sleep)
{
	p->sched_class->dequeue_task(rq, p, sleep);
	p->se.on_rq = 0;
}

/*
 * __normal_prio - return the priority that is based on the static prio
 */
static inline int __normal_prio(struct task_struct *p)
{
	return p->static_prio;
}

/*
 * Calculate the expected normal priority: i.e. priority
 * without taking RT-inheritance into account. Might be
 * boosted by interactivity modifiers. Changes upon fork,
 * setprio syscalls, and whenever the interactivity
 * estimator recalculates.
 */
static inline int normal_prio(struct task_struct *p)
{
	int prio;

	if (task_has_rt_policy(p))
		prio = MAX_RT_PRIO-1 - p->rt_priority;
	else
		prio = __normal_prio(p);
	return prio;
}

/*
 * Calculate the current priority, i.e. the priority
 * taken into account by the scheduler. This value might
 * be boosted by RT tasks, or might be boosted by
 * interactivity modifiers. Will be RT if the task got
 * RT-boosted. If not then it returns p->normal_prio.
 */
static int effective_prio(struct task_struct *p)
{
	p->normal_prio = normal_prio(p);
	/*
	 * If we are RT tasks or we were boosted to RT priority,
	 * keep the priority unchanged. Otherwise, update priority
	 * to the normal priority:
	 */
	if (!rt_prio(p->prio))
		return p->normal_prio;
	return p->prio;
}

/*
 * activate_task - move a task to the runqueue.
 */
static void activate_task(struct rq *rq, struct task_struct *p, int wakeup)
{
	if (p->state == TASK_UNINTERRUPTIBLE)
		rq->nr_uninterruptible--;

	enqueue_task(rq, p, wakeup);
	inc_nr_running(p, rq);
}

/*
 * deactivate_task - remove a task from the runqueue.
 */
static void deactivate_task(struct rq *rq, struct task_struct *p, int sleep)
{
	if (p->state == TASK_UNINTERRUPTIBLE)
		rq->nr_uninterruptible++;

	dequeue_task(rq, p, sleep);
	dec_nr_running(p, rq);
}

/**
 * task_curr - is this task currently executing on a CPU?
 * @p: the task in question.
 */
inline int task_curr(const struct task_struct *p)
{
	return cpu_curr(task_cpu(p)) == p;
}

/* Used instead of source_load when we know the type == 0 */
unsigned long weighted_cpuload(const int cpu)
{
	return cpu_rq(cpu)->load.weight;
}

static inline void __set_task_cpu(struct task_struct *p, unsigned int cpu)
{
	/*
	 * After ->cpu is set up to a new value, task_rq_lock(p, ...) can be
	 * successfuly executed on another CPU. We must ensure that updates of
	 * per-task data have been completed by this moment.
	 */
	smp_wmb();
	p->cpu = cpu;
}

/*
 * Is this task likely cache-hot:
 */
static inline int
task_hot(struct task_struct *p, u64 now, struct sched_domain *sd)
{
	s64 delta;

	if (p->sched_class != &fair_sched_class)
		return 0;

	if (sysctl_sched_migration_cost == -1)
		return 1;
	if (sysctl_sched_migration_cost == 0)
		return 0;

	delta = now - p->se.exec_start;

	return delta < (s64)sysctl_sched_migration_cost;
}

void set_task_cpu(struct task_struct *p, unsigned int new_cpu)
{
	int old_cpu = task_cpu(p);
	struct rq *old_rq = cpu_rq(old_cpu), *new_rq = cpu_rq(new_cpu);
	struct cfs_rq *old_cfsrq = task_cfs_rq(p),
		      *new_cfsrq = cpu_cfs_rq(old_cfsrq, new_cpu);
	u64 clock_offset;

	clock_offset = old_rq->clock - new_rq->clock;

	p->se.vruntime -= old_cfsrq->min_vruntime -
					 new_cfsrq->min_vruntime;

	__set_task_cpu(p, new_cpu);
}

struct migration_req {
	struct list_head list;

	struct task_struct *task;
	int dest_cpu;

	struct completion done;
};

/*
 * The task's runqueue lock must be held.
 * Returns true if you have to wait for migration thread.
 */
static int
migrate_task(struct task_struct *p, int dest_cpu, struct migration_req *req)
{
	struct rq *rq = task_rq(p);

	/*
	 * If the task is not on a runqueue (and not running), then
	 * it is sufficient to simply update the task's cpu field.
	 */
	if (!p->se.on_rq && !task_running(rq, p)) {
		set_task_cpu(p, dest_cpu);
		return 0;
	}

	init_completion(&req->done);
	req->task = p;
	req->dest_cpu = dest_cpu;
	list_add(&req->list, &rq->migration_queue);

	return 1;
}

/*
 * wait_task_inactive - wait for a thread to unschedule.
 *
 * The caller must ensure that the task *will* unschedule sometime soon,
 * else this function might spin for a *long* time. This function can't
 * be called with interrupts off, or it may introduce deadlock with
 * smp_call_function() if an IPI is sent by the same process we are
 * waiting to become inactive.
 */
void wait_task_inactive(struct task_struct *p)
{
	u64 flags;
	int running, on_rq;
	struct rq *rq;

	for (;;) {
		/*
		 * We do the initial early heuristics without holding
		 * any task-queue locks at all. We'll only try to get
		 * the runqueue lock when things look like they will
		 * work out!
		 */
		rq = task_rq(p);

		/*
		 * If the task is actively running on another CPU
		 * still, just relax and busy-wait without holding
		 * any locks.
		 *
		 * NOTE! Since we don't hold any locks, it's not
		 * even sure that "rq" stays as the right runqueue!
		 * But we don't care, since "task_running()" will
		 * return false if the runqueue has changed and p
		 * is actually now running somewhere else!
		 */
		while (task_running(rq, p))
			cpu_relax();

		/*
		 * Ok, time to look more closely! We need the rq
		 * lock now, to be *sure*. If we're wrong, we'll
		 * just go back and repeat.
		 */
		rq = task_rq_lock(p, &flags);
		running = task_running(rq, p);
		on_rq = p->se.on_rq;
		task_rq_unlock(rq, &flags);

		/*
		 * Was it really running after all now that we
		 * checked with the proper locks actually held?
		 *
		 * Oops. Go back and try again..
		 */
		if (unlikely(running)) {
			cpu_relax();
			continue;
		}

		/*
		 * It's not enough that it's not actively running,
		 * it must be off the runqueue _entirely_, and not
		 * preempted!
		 *
		 * So if it wa still runnable (but just not actively
		 * running right now), it's preempted, and we should
		 * yield - it could be a while.
		 */
		if (unlikely(on_rq)) {
			schedule_timeout_uninterruptible(1);
			continue;
		}

		/*
		 * Ahh, all good. It wasn't running, and it wasn't
		 * runnable, which means that it will never become
		 * running in the future either. We're all done!
		 */
		break;
	}
}

/***
 * kick_process - kick a running thread to enter/exit the kernel
 * @p: the to-be-kicked thread
 *
 * Cause a process which is running on another CPU to enter
 * kernel-mode, without any delay. (to get signals handled.)
 *
 * NOTE: this function doesnt have to take the runqueue lock,
 * because all it wants to ensure is that the remote task enters
 * the kernel. If the IPI races and the task has been migrated
 * to another CPU then no harm is done and the purpose has been
 * achieved as well.
 */
void kick_process(struct task_struct *p)
{
	int cpu;

	preempt_disable();
	cpu = task_cpu(p);
	if ((cpu != smp_processor_id()) && task_curr(p))
		smp_send_reschedule(cpu);
	preempt_enable();
}

/*
 * Return a low guess at the load of a migration-source cpu weighted
 * according to the scheduling class and "nice" value.
 *
 * We want to under-estimate the load of migration sources, to
 * balance conservatively.
 */
static unsigned long source_load(int cpu, int type)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long total = weighted_cpuload(cpu);

	if (type == 0)
		return total;

	return min(rq->cpu_load[type-1], total);
}

/*
 * Return a high guess at the load of a migration-target cpu weighted
 * according to the scheduling class and "nice" value.
 */
static unsigned long target_load(int cpu, int type)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long total = weighted_cpuload(cpu);

	if (type == 0)
		return total;

	return max(rq->cpu_load[type-1], total);
}

/*
 * Return the average load per task on the cpu's run queue
 */
static inline unsigned long cpu_avg_load_per_task(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long total = weighted_cpuload(cpu);
	unsigned long n = rq->nr_running;

	return n ? total / n : SCHED_LOAD_SCALE;
}

/*
 * find_idlest_group finds and returns the least busy CPU group within the
 * domain.
 */
static struct sched_group *
find_idlest_group(struct sched_domain *sd, struct task_struct *p, int this_cpu)
{
	struct sched_group *idlest = NULL, *this = NULL, *group = sd->groups;
	unsigned long min_load = ULONG_MAX, this_load = 0;
	int load_idx = sd->forkexec_idx;
	int imbalance = 100 + (sd->imbalance_pct-100)/2;

	do {
		unsigned long load, avg_load;
		int local_group;
		int i;

		/* Skip over this group if it has no CPUs allowed */
		if (!cpumask_intersects(&group->cpumask, &p->cpus_allowed))
			continue;

		local_group = cpumask_test_cpu(this_cpu, &group->cpumask);

		/* Tally up the load of all CPUs in the group */
		avg_load = 0;

		for_each_cpu(i, &group->cpumask) {
			/* Bias balancing toward cpus of our domain */
			if (local_group)
				load = source_load(i, load_idx);
			else
				load = target_load(i, load_idx);

			avg_load += load;
		}

		/* Adjust by relative CPU power of the group */
		avg_load = sg_div_cpu_power(group,
				avg_load * SCHED_LOAD_SCALE);

		if (local_group) {
			this_load = avg_load;
			this = group;
		} else if (avg_load < min_load) {
			min_load = avg_load;
			idlest = group;
		}
	} while (group = group->next, group != sd->groups);

	if (!idlest || 100*this_load < imbalance*min_load)
		return NULL;
	return idlest;
}

/*
 * find_idlest_cpu - find the idlest cpu among the cpus in group.
 */
static int
find_idlest_cpu(struct sched_group *group, struct task_struct *p, int this_cpu)
{
	cpumask_t tmp;
	unsigned long load, min_load = ULONG_MAX;
	int idlest = -1;
	int i;

	/* Traverse only the allowed CPUs */
	cpumask_and(&tmp, &group->cpumask, &p->cpus_allowed);

	for_each_cpu(i, &tmp) {
		load = weighted_cpuload(i);

		if (load < min_load || (load == min_load && i == this_cpu)) {
			min_load = load;
			idlest = i;
		}
	}

	return idlest;
}

/*
 * sched_balance_self: balance the current task (running on cpu) in domains
 * that have the 'flag' flag set. In practice, this is SD_BALANCE_FORK and
 * SD_BALANCE_EXEC.
 *
 * Balance, ie. select the least loaded group.
 *
 * Returns the target CPU number, or the same CPU if no balancing is needed.
 *
 * preempt must be disabled.
 */
static int sched_balance_self(int cpu, int flag)
{
	struct task_struct *t = current;
	struct sched_domain *tmp, *sd = NULL;

	for_each_domain(cpu, tmp) {
		/*
		 * If power savings logic is enabled for a domain, stop there.
		 */
		if (tmp->flags & SD_POWERSAVINGS_BALANCE)
			break;
		if (tmp->flags & flag)
			sd = tmp;
	}

	while (sd) {
		cpumask_t span;
		struct sched_group *group;
		int new_cpu, weight;

		if (!(sd->flags & flag)) {
			sd = sd->child;
			continue;
		}

		span = sd->span;
		group = find_idlest_group(sd, t, cpu);
		if (!group) {
			sd = sd->child;
			continue;
		}

		new_cpu = find_idlest_cpu(group, t, cpu);
		if (new_cpu == -1 || new_cpu == cpu) {
			/* Now try balancing at a lower domain level of cpu */
			sd = sd->child;
			continue;
		}

		/* Now try balancing at a lower domain level of new_cpu */
		cpu = new_cpu;
		sd = NULL;
		weight = cpumask_weight(&span);
		for_each_domain(cpu, tmp) {
			if (weight <= cpumask_weight(&tmp->span))
				break;
			if (tmp->flags & flag)
				sd = tmp;
		}
		/* while loop will break here if sd == NULL */
	}

	return cpu;
}

static inline int wake_idle(int cpu, struct task_struct *p)
{
	return cpu;
}

/***
 * try_to_wake_up - wake up a thread
 * @p: the to-be-woken-up thread
 * @state: the mask of task states that can be woken
 * @sync: do a synchronous wakeup?
 *
 * Put it on the run-queue if it's not already there. The "current"
 * thread is always on the run-queue (except when the actual
 * re-schedule is in progress), and as such you're allowed to do
 * the simpler "current->state = TASK_RUNNING" to mark yourself
 * runnable without the overhead of this.
 *
 * returns failure only if the task is already active.
 */
static int try_to_wake_up(struct task_struct *p, unsigned int state, int sync)
{
	int cpu, orig_cpu, this_cpu, success = 0;
	u64 flags;
	long old_state;
	struct rq *rq;
	struct sched_domain *sd, *this_sd = NULL;
	unsigned long load, this_load;
	int new_cpu;

	rq = task_rq_lock(p, &flags);
	old_state = p->state;
	if (!(old_state & state))
		goto out;

	if (p->se.on_rq)
		goto out_running;

	cpu = task_cpu(p);
	orig_cpu = cpu;
	this_cpu = smp_processor_id();

	if (unlikely(task_running(rq, p)))
		goto out_activate;

	new_cpu = cpu;

	if (cpu == this_cpu)
		goto out_set_cpu;

	for_each_domain(this_cpu, sd) {
		if (cpumask_test_cpu(cpu, &sd->span)) {
			this_sd = sd;
			break;
		}
	}

	if (unlikely(!cpumask_test_cpu(this_cpu, &p->cpus_allowed)))
		goto out_set_cpu;

	/*
	 * Check for affine wakeup and passive balancing possibilities.
	 */
	if (this_sd) {
		int idx = this_sd->wake_idx;
		unsigned int imbalance;

		imbalance = 100 + (this_sd->imbalance_pct - 100) / 2;

		load = source_load(cpu, idx);
		this_load = target_load(this_cpu, idx);

		new_cpu = this_cpu; /* Wake to this CPU if we can */

		if (this_sd->flags & SD_WAKE_AFFINE) {
			unsigned long tl = this_load;
			unsigned long tl_per_task;

			/*
			 * Attract cache-cold tasks on sync wakeups:
			 */
			if (sync && !task_hot(p, rq->clock, this_sd))
				goto out_set_cpu;

			tl_per_task = cpu_avg_load_per_task(this_cpu);

			/*
			 * If sync wakeup then subtract the (maximum possible)
			 * effect of the currently running task from the load
			 * of the current CPU:
			 */
			if (sync)
				tl -= current->se.load.weight;

			if ((tl <= load &&
				tl + target_load(cpu, idx) <= tl_per_task) ||
			       100*(tl + p->se.load.weight) <= imbalance*load) {
				goto out_set_cpu;
			}
		}

		/*
		 * Start passive balancing when half the imbalance_pct
		 * limit is reached.
		 */
		if (this_sd->flags & SD_WAKE_BALANCE) {
			if (imbalance*this_load <= 100*load) {
				goto out_set_cpu;
			}
		}
	}

	new_cpu = cpu; /* Could not wake to this_cpu. Wake to cpu instead */
out_set_cpu:
	new_cpu = wake_idle(new_cpu, p);
	if (new_cpu != cpu) {
		set_task_cpu(p, new_cpu);
		task_rq_unlock(rq, &flags);
		/* might preempt at this point */
		rq = task_rq_lock(p, &flags);
		old_state = p->state;
		if (!(old_state & state))
			goto out;
		if (p->se.on_rq)
			goto out_running;

		this_cpu = smp_processor_id();
		cpu = task_cpu(p);
	}

out_activate:
	update_rq_clock(rq);
	activate_task(rq, p, 1);
	check_preempt_curr(rq, p);
	success = 1;

out_running:
	p->state = TASK_RUNNING;
out:
	task_rq_unlock(rq, &flags);

	return success;
}

int wake_up_process(struct task_struct *p)
{
	return try_to_wake_up(p, TASK_STOPPED | TASK_TRACED |
				 TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE, 0);
}

int wake_up_state(struct task_struct *p, unsigned int state)
{
	return try_to_wake_up(p, state, 0);
}

/*
 * Perform scheduler related setup for a newly forked process p.
 * p is forked by current.
 *
 * __sched_fork() is basic setup used by init_idle() too:
 */
static void __sched_fork(struct task_struct *p)
{
	p->se.exec_start		= 0;
	p->se.sum_exec_runtime		= 0;
	p->se.prev_sum_exec_runtime	= 0;

	INIT_LIST_HEAD(&p->run_list);
	p->se.on_rq = 0;

	/*
	 * We mark the process as running here, but have not actually
	 * inserted it onto the runqueue yet. This guarantees that
	 * nobody will actually run it, and a signal or other external
	 * event cannot wake it up and insert it on the runqueue either.
	 */
	p->state = TASK_RUNNING;
}

/*
 * fork()/clone()-time setup:
 */
void sched_fork(struct task_struct *p, int clone_flags)
{
	int cpu = smp_processor_id();

	preempt_disable();

	__sched_fork(p);

	cpu = sched_balance_self(cpu, SD_BALANCE_FORK);

	set_task_cpu(p, cpu);

	/*
	 * Make sure we do not leak PI boosting priority to the child:
	 */
	p->prio = current->normal_prio;
	if (!rt_prio(p->prio))
		p->sched_class = &fair_sched_class;


#if defined(__ARCH_WANT_UNLOCKED_CTXSW)
	p->oncpu = 0;
#endif
	/* Want to start with kernel preemption disabled. */
	task_thread_info(p)->preempt_count = 1;

	preempt_enable();
}

/*
 * wake_up_new_task - wake up a newly created task for the first time.
 *
 * This function will do some initial scheduler statistics housekeeping
 * that must be done for every newly created context, then puts the task
 * on the runqueue and wakes it.
 */
void wake_up_new_task(struct task_struct *p, unsigned long clone_flags)
{
	u64 flags;
	struct rq *rq;

	rq = task_rq_lock(p, &flags);
	BUG_ON(p->state != TASK_RUNNING);
	update_rq_clock(rq);

	p->prio = effective_prio(p);

	if (!p->sched_class->task_new || !current->se.on_rq) {
		activate_task(rq, p, 0);
	} else {
		/*
		 * Let the scheduling class do new task startup
		 * management (if any):
		 */
		p->sched_class->task_new(rq, p);
		inc_nr_running(p, rq);
	}
	check_preempt_curr(rq, p);
	task_rq_unlock(rq, &flags);
}

/**
 * prepare_task_switch - prepare to switch tasks
 * @rq: the runqueue preparing to switch
 * @prev: the current task that is being switched out
 * @next: the task we are going to switch to.
 *
 * This is called with the rq lock held and interrupts off. It must
 * be paired with a subsequent finish_task_switch after the context
 * switch.
 *
 * prepare_task_switch sets up locking and calls architecture specific
 * hooks.
 */
static inline void
prepare_task_switch(struct rq *rq, struct task_struct *prev,
		    struct task_struct *next)
{
	prepare_lock_switch(rq, next);
	prepare_arch_switch(next);
}

/**
 * finish_task_switch - clean up after a task-switch
 * @rq: runqueue associated with task-switch
 * @prev: the thread we just switched away from.
 *
 * finish_task_switch must be called after the context switch, paired
 * with a prepare_task_switch call before the context switch.
 * finish_task_switch will reconcile locking set up by prepare_task_switch,
 * and do any other architecture-specific cleanup actions.
 *
 * Note that we may have delayed dropping an mm in context_switch(). If
 * so, we finish that here outside of the runqueue lock. (Doing it
 * with the lock held can cause deadlocks; see schedule() for
 * details.)
 */
static void finish_task_switch(struct rq *rq, struct task_struct *prev)
	__releases(rq->lock)
{
	struct mm_struct *mm = rq->prev_mm;
	long prev_state;

	rq->prev_mm = NULL;

	/*
	 * A task struct has one reference for the use as "current".
	 * If a task dies, then it sets TASK_DEAD in tsk->state and calls
	 * schedule one last time. The schedule call will never return, and
	 * the scheduled task must drop that reference.
	 * The test for TASK_DEAD must occur while the runqueue locks are
	 * still held, otherwise prev could be scheduled on another cpu, die
	 * there before we look at prev->state, and then the reference would
	 * be dropped twice.
	 *		Manfred Spraul <manfred@colorfullife.com>
	 */
	prev_state = prev->state;
	finish_arch_switch(prev);
	finish_lock_switch(rq, prev);
	if (mm)
		; /* TODO */
	if (unlikely(prev_state == TASK_DEAD)) {
		put_task_struct(prev);
	}
}

/**
 * schedule_tail - first thing a freshly forked thread must call.
 * @prev: the thread we just switched away from.
 */
asmlinkage void schedule_tail(struct task_struct *prev)
	__releases(rq->lock)
{
	struct rq *rq = this_rq();

	finish_task_switch(rq, prev);
#ifdef __ARCH_WANT_UNLOCKED_CTXSW
	/* In this case, finish_task_switch does not reenable preemption */
	preempt_enable();
#endif
}

/*
 * context_switch - switch to the new MM and the new
 * thread's register state.
 */
static inline void
context_switch(struct rq *rq, struct task_struct *prev,
	       struct task_struct *next)
{
	struct mm_struct *mm, *oldmm;

	prepare_task_switch(rq, prev, next);
	mm = next->mm;
	oldmm = prev->mm;

	switch_mm(oldmm, mm, next);

	if (unlikely(!prev->mm))
		rq->prev_mm = oldmm;

	/*
	 * Since the runqueue lock will be released by the next
	 * task (which is an invalid locking op but in the case
	 * of the scheduler it's an obvious special-case), so we
	 * do an early lockdep release here:
	 */
#ifndef __ARCH_WANT_UNLOCKED_CTXSW
	spin_release(&rq->lock.dep_map, 1, _THIS_IP_);
#endif

	/* Here we just switch the register state and the stack. */
	switch_to(prev, next, prev);

	barrier();
	/*
	 * this_rq must be evaluated again because prev may have moved
	 * CPUs since it called schedule(), thus the 'rq' on its stack
	 * frame will be invalid.
	 */
	finish_task_switch(this_rq(), prev);
}

/*
 * nr_running, nr_uninterruptible and nr_context_switches:
 *
 * externally visible scheduler statistics: current number of runnable
 * threads, current number of uninterruptible-sleeping threads, total
 * number of context switches performed since bootup.
 */
unsigned long nr_running(void)
{
	unsigned long i, sum = 0;

	for_each_online_cpu(i)
		sum += cpu_rq(i)->nr_running;

	return sum;
}

unsigned long nr_uninterruptible(void)
{
	unsigned long i, sum = 0;

	for_each_possible_cpu(i)
		sum += cpu_rq(i)->nr_uninterruptible;

	/*
	 * Since we read the counters lockless, it might be slightly
	 * inaccurate. Do not allow it to go below zero though:
	 */
	if (unlikely((long)sum < 0))
		sum = 0;

	return sum;
}

unsigned long long nr_context_switches(void)
{
	int i;
	unsigned long long sum = 0;

	for_each_possible_cpu(i)
		sum += cpu_rq(i)->nr_switches;

	return sum;
}

unsigned long nr_iowait(void)
{
	unsigned long i, sum = 0;

	for_each_possible_cpu(i)
		sum += atomic_read(&cpu_rq(i)->nr_iowait);

	return sum;
}

unsigned long nr_active(void)
{
	unsigned long i, running = 0, uninterruptible = 0;

	for_each_online_cpu(i) {
		running += cpu_rq(i)->nr_running;
		uninterruptible += cpu_rq(i)->nr_uninterruptible;
	}

	if (unlikely((long)uninterruptible < 0))
		uninterruptible = 0;

	return running + uninterruptible;
}

/*
 * Update rq->cpu_load[] statistics. This function is usually called every
 * scheduler tick (TICK_NSEC).
 */
static void update_cpu_load(struct rq *this_rq)
{
	unsigned long this_load = this_rq->load.weight;
	int i, scale;

	this_rq->nr_load_updates++;

	/* Update our load: */
	for (i = 0, scale = 1; i < CPU_LOAD_IDX_MAX; i++, scale += scale) {
		unsigned long old_load, new_load;

		/* scale is effectively 1 << i now, and >> i divides by scale */

		old_load = this_rq->cpu_load[i];
		new_load = this_load;
		/*
		 * Round up the averaging division if load is increasing. This
		 * prevents us from getting stuck on 9 if the load is 10, for
		 * example.
		 */
		if (new_load > old_load)
			new_load += scale-1;
		this_rq->cpu_load[i] = (old_load*(scale-1) + new_load) >> i;
	}
}

/*
 * double_rq_lock - safely lock two runqueues
 *
 * Note this does not disable interrupts like task_rq_lock,
 * you need to do so manually before calling.
 */
static void double_rq_lock(struct rq *rq1, struct rq *rq2)
	__acquires(rq1->lock)
	__acquires(rq2->lock)
{
	BUG_ON(!irqs_disabled());
	if (rq1 == rq2) {
		spin_lock(&rq1->lock);
		__acquire(rq2->lock);	/* Fake it out ;) */
	} else {
		if (rq1 < rq2) {
			spin_lock(&rq1->lock);
			spin_lock(&rq2->lock);
		} else {
			spin_lock(&rq2->lock);
			spin_lock(&rq1->lock);
		}
	}
	update_rq_clock(rq1);
	update_rq_clock(rq2);
}

/*
 * double_rq_unlock - safely unlock two runqueues
 *
 * Note this does not restore interrupts like task_rq_unlock,
 * you need to do so manually after calling.
 */
static void double_rq_unlock(struct rq *rq1, struct rq *rq2)
	__releases(rq1->lock)
	__releases(rq2->lock)
{
	spin_unlock(&rq1->lock);
	if (rq1 != rq2)
		spin_unlock(&rq2->lock);
	else
		__release(rq2->lock);
}

/*
 * double_lock_balance - lock the busiest runqueue, this_rq is locked already.
 */
static void double_lock_balance(struct rq *this_rq, struct rq *busiest)
	__releases(this_rq->lock)
	__acquires(busiest->lock)
	__acquires(this_rq->lock)
{
	if (unlikely(!irqs_disabled())) {
		/* printf() doesn't work good under rq->lock */
		spin_unlock(&this_rq->lock);
		BUG_ON(1);
	}
	if (unlikely(!spin_trylock(&busiest->lock))) {
		if (busiest < this_rq) {
			spin_unlock(&this_rq->lock);
			spin_lock(&busiest->lock);
			spin_lock(&this_rq->lock);
		} else
			spin_lock(&busiest->lock);
	}
}

/*
 * If dest_cpu is allowed for this process, migrate the task to it.
 * This is accomplished by forcing the cpu_allowed mask to only
 * allow dest_cpu, which will force the cpu onto dest_cpu. Then
 * the cpu_allowed mask is restored.
 */
static void sched_migrate_task(struct task_struct *p, int dest_cpu)
{
	struct migration_req req;
	u64 flags;
	struct rq *rq;

	rq = task_rq_lock(p, &flags);
	if (!cpumask_test_cpu(dest_cpu, &p->cpus_allowed)
	    || unlikely(cpu_is_offline(dest_cpu)))
		goto out;

	/* force the process onto the specified CPU */
	if (migrate_task(p, dest_cpu, &req)) {
		/* Need to wait for migration thread (might exit: take ref). */
		struct task_struct *mt = rq->migration_thread;

		get_task_struct(mt);
		task_rq_unlock(rq, &flags);
		wake_up_process(mt);
		put_task_struct(mt);
		wait_for_completion(&req.done);

		return;
	}
out:
	task_rq_unlock(rq, &flags);
}

/*
 * sched_exec - execve() is a valuable balancing opportunity, because at
 * this point the task has the smallest effective memory and cache footprint.
 */
void sched_exec(void)
{
	int new_cpu, this_cpu;

	this_cpu = smp_processor_id();

	preempt_disable();

	new_cpu = sched_balance_self(this_cpu, SD_BALANCE_EXEC);
	preempt_enable();

	if (new_cpu != this_cpu)
		sched_migrate_task(current, new_cpu);
}

/*
 * pull_task - move a task from a remote runqueue to the local runqueue.
 * Both runqueues must be locked.
 */
static void pull_task(struct rq *src_rq, struct task_struct *p,
		      struct rq *this_rq, int this_cpu)
{
	deactivate_task(src_rq, p, 0);
	set_task_cpu(p, this_cpu);
	activate_task(this_rq, p, 0);
	/*
	 * Note that idle threads have a prio of MAX_PRIO, for this test
	 * to be always true for them.
	 */
	check_preempt_curr(this_rq, p);
}

/*
 * can_migrate_task - may task p from runqueue rq be migrated to this_cpu?
 */
static
int can_migrate_task(struct task_struct *p, struct rq *rq, int this_cpu,
		     struct sched_domain *sd, enum cpu_idle_type idle,
		     int *all_pinned)
{
	/*
	 * We do not migrate tasks that are:
	 * 1) running (obviously), or
	 * 2) cannot be migrated to this CPU due to cpus_allowed, or
	 * 3) are cache-hot on their current CPU.
	 */
	if (!cpumask_test_cpu(this_cpu, &p->cpus_allowed))
		return 0;

	*all_pinned = 0;

	if (task_running(rq, p))
		return 0;

	/*
	 * Aggressive migration if:
	 * 1) task is cache cold, or
	 * 2) too many balance attempts have failed.
	 */

	if (!task_hot(p, rq->clock, sd) ||
			sd->nr_balance_failed > sd->cache_nice_tries)
		return 1;

	if (task_hot(p, rq->clock, sd))
		return 0;

	return 1;
}

unsigned long
balance_tasks(struct rq *this_rq, int this_cpu, struct rq *busiest,
	      unsigned long max_load_move, struct sched_domain *sd,
	      enum cpu_idle_type idle, int *all_pinned,
	      int *this_best_prio, struct rq_iterator *iterator)
{
	int loops = 0, pulled = 0, pinned = 0, skip_for_load;
	struct task_struct *p;
	long rem_load_move = max_load_move;

	if (max_load_move == 0)
		goto out;

	pinned = 1;

	/*
	 * Start the load-balancing iterator:
	 */
	p = iterator->start(iterator->arg);
next:
	if (!p || loops++ > sysctl_sched_nr_migrate)
		goto out;
	/*
	 * To help distribute high priority tasks across CPUs we don't
	 * skip a task if it will be the highest priority task (i.e. smallest
	 * prio value) on its new queue regardless of its load weight
	 */
	skip_for_load = (p->se.load.weight >> 1) > rem_load_move +
							 SCHED_LOAD_SCALE_FUZZ;
	if ((skip_for_load && p->prio >= *this_best_prio) ||
	    !can_migrate_task(p, busiest, this_cpu, sd, idle, &pinned)) {
		p = iterator->next(iterator->arg);
		goto next;
	}

	pull_task(busiest, p, this_rq, this_cpu);
	pulled++;
	rem_load_move -= p->se.load.weight;

	/*
	 * We only want to steal up to the prescribed amount of weighted load.
	 */
	if (rem_load_move > 0) {
		if (p->prio < *this_best_prio)
			*this_best_prio = p->prio;
		p = iterator->next(iterator->arg);
		goto next;
	}
out:
	if (all_pinned)
		*all_pinned = pinned;

	return max_load_move - rem_load_move;
}

/*
 * move_tasks tries to move up to max_load_move weighted load from busiest to
 * this_rq, as part of a balancing operation within domain "sd".
 * Returns 1 if successful and 0 otherwise.
 *
 * Called with both runqueues locked.
 */
static int move_tasks(struct rq *this_rq, int this_cpu, struct rq *busiest,
		      unsigned long max_load_move,
		      struct sched_domain *sd, enum cpu_idle_type idle,
		      int *all_pinned)
{
	const struct sched_class *class = sched_class_highest;
	unsigned long total_load_moved = 0;
	int this_best_prio = this_rq->curr->prio;

	do {
		total_load_moved +=
			class->load_balance(this_rq, this_cpu, busiest,
				max_load_move - total_load_moved,
				sd, idle, all_pinned, &this_best_prio);
		class = class->next;
	} while (class && max_load_move > total_load_moved);

	return total_load_moved > 0;
}

int
iter_move_one_task(struct rq *this_rq, int this_cpu, struct rq *busiest,
		   struct sched_domain *sd, enum cpu_idle_type idle,
		   struct rq_iterator *iterator)
{
	struct task_struct *p = iterator->start(iterator->arg);
	int pinned = 0;

	while (p) {
		if (can_migrate_task(p, busiest, this_cpu, sd, idle, &pinned)) {
			pull_task(busiest, p, this_rq, this_cpu);

			return 1;
		}
		p = iterator->next(iterator->arg);
	}

	return 0;
}

/*
 * find_busiest_group finds and returns the busiest CPU group within the
 * domain. It calculates and returns the amount of weighted load which
 * should be moved to restore balance via the imbalance parameter.
 */
static struct sched_group *
find_busiest_group(struct sched_domain *sd, int this_cpu,
		   unsigned long *imbalance, enum cpu_idle_type idle,
		   int *sd_idle, cpumask_t *cpus, int *balance)
{
	struct sched_group *busiest = NULL, *this = NULL, *group = sd->groups;
	unsigned long max_load, avg_load, total_load, this_load, total_pwr;
	unsigned long max_pull;
	unsigned long busiest_load_per_task, busiest_nr_running;
	unsigned long this_load_per_task, this_nr_running;
	int load_idx, group_imb = 0;

	max_load = this_load = total_load = total_pwr = 0;
	busiest_load_per_task = busiest_nr_running = 0;
	this_load_per_task = this_nr_running = 0;
	if (idle == CPU_NOT_IDLE)
		load_idx = sd->busy_idx;
	else if (idle == CPU_NEWLY_IDLE)
		load_idx = sd->newidle_idx;
	else
		load_idx = sd->idle_idx;

	do {
		unsigned long load, group_capacity, max_cpu_load, min_cpu_load;
		int local_group;
		int i;
		int __group_imb = 0;
		unsigned int balance_cpu = -1, first_idle_cpu = 0;
		unsigned long sum_nr_running, sum_weighted_load;

		local_group = cpumask_test_cpu(this_cpu, &group->cpumask);

		if (local_group)
			balance_cpu = cpumask_first(&group->cpumask);

		/* Tally up the load of all CPUs in the group */
		sum_weighted_load = sum_nr_running = avg_load = 0;
		max_cpu_load = 0;
		min_cpu_load = ~0UL;

		for_each_cpu(i, &group->cpumask) {
			struct rq *rq;

			if (!cpumask_test_cpu(i, cpus))
				continue;

			rq = cpu_rq(i);

			if (*sd_idle && rq->nr_running)
				*sd_idle = 0;

			/* Bias balancing toward cpus of our domain */
			if (local_group) {
				if (idle_cpu(i) && !first_idle_cpu) {
					first_idle_cpu = 1;
					balance_cpu = i;
				}

				load = target_load(i, load_idx);
			} else {
				load = source_load(i, load_idx);
				if (load > max_cpu_load)
					max_cpu_load = load;
				if (min_cpu_load > load)
					min_cpu_load = load;
			}

			avg_load += load;
			sum_nr_running += rq->nr_running;
			sum_weighted_load += weighted_cpuload(i);
		}

		/*
		 * First idle cpu or the first cpu(busiest) in this sched group
		 * is eligible for doing load balancing at this and above
		 * domains. In the newly idle case, we will allow all the cpu's
		 * to do the newly idle load balance.
		 */
		if (idle != CPU_NEWLY_IDLE && local_group &&
		    balance_cpu != this_cpu && balance) {
			*balance = 0;
			goto ret;
		}

		total_load += avg_load;
		total_pwr += group->__cpu_power;

		/* Adjust by relative CPU power of the group */
		avg_load = sg_div_cpu_power(group,
				avg_load * SCHED_LOAD_SCALE);

		if ((max_cpu_load - min_cpu_load) > SCHED_LOAD_SCALE)
			__group_imb = 1;

		group_capacity = group->__cpu_power / SCHED_LOAD_SCALE;

		if (local_group) {
			this_load = avg_load;
			this = group;
			this_nr_running = sum_nr_running;
			this_load_per_task = sum_weighted_load;
		} else if (avg_load > max_load &&
			   (sum_nr_running > group_capacity || __group_imb)) {
			max_load = avg_load;
			busiest = group;
			busiest_nr_running = sum_nr_running;
			busiest_load_per_task = sum_weighted_load;
			group_imb = __group_imb;
		}

		group = group->next;
	} while (group != sd->groups);

	if (!busiest || this_load >= max_load || busiest_nr_running == 0)
		goto out_balanced;

	avg_load = (SCHED_LOAD_SCALE * total_load) / total_pwr;

	if (this_load >= avg_load ||
			100*max_load <= sd->imbalance_pct*this_load)
		goto out_balanced;

	busiest_load_per_task /= busiest_nr_running;
	if (group_imb)
		busiest_load_per_task = min(busiest_load_per_task, avg_load);

	/*
	 * We're trying to get all the cpus to the average_load, so we don't
	 * want to push ourselves above the average load, nor do we wish to
	 * reduce the max loaded cpu below the average load, as either of these
	 * actions would just result in more rebalancing later, and ping-pong
	 * tasks around. Thus we look for the minimum possible imbalance.
	 * Negative imbalances (*we* are more loaded than anyone else) will
	 * be counted as no imbalance for these purposes -- we can't fix that
	 * by pulling tasks to us. Be careful of negative numbers as they'll
	 * appear as very large values with unsigned longs.
	 */
	if (max_load <= busiest_load_per_task)
		goto out_balanced;

	/*
	 * In the presence of smp nice balancing, certain scenarios can have
	 * max load less than avg load(as we skip the groups at or below
	 * its cpu_power, while calculating max_load..)
	 */
	if (max_load < avg_load) {
		*imbalance = 0;
		goto small_imbalance;
	}

	/* Don't want to pull so many tasks that a group would go idle */
	max_pull = min(max_load - avg_load, max_load - busiest_load_per_task);

	/* How much load to actually move to equalise the imbalance */
	*imbalance = min(max_pull * busiest->__cpu_power,
				(avg_load - this_load) * this->__cpu_power)
			/ SCHED_LOAD_SCALE;

	/*
	 * if *imbalance is less than the average load per runnable task
	 * there is no gaurantee that any tasks will be moved so we'll have
	 * a think about bumping its value to force at least one task to be
	 * moved
	 */
	if (*imbalance < busiest_load_per_task) {
		unsigned long tmp, pwr_now, pwr_move;
		unsigned int imbn;

small_imbalance:
		pwr_move = pwr_now = 0;
		imbn = 2;
		if (this_nr_running) {
			this_load_per_task /= this_nr_running;
			if (busiest_load_per_task > this_load_per_task)
				imbn = 1;
		} else
			this_load_per_task = SCHED_LOAD_SCALE;

		if (max_load - this_load + SCHED_LOAD_SCALE_FUZZ >=
					busiest_load_per_task * imbn) {
			*imbalance = busiest_load_per_task;
			return busiest;
		}

		/*
		 * OK, we don't have enough imbalance to justify moving tasks,
		 * however we may be able to increase total CPU power used by
		 * moving them.
		 */

		pwr_now += busiest->__cpu_power *
				min(busiest_load_per_task, max_load);
		pwr_now += this->__cpu_power *
				min(this_load_per_task, this_load);
		pwr_now /= SCHED_LOAD_SCALE;

		/* Amount of load we'd subtract */
		tmp = sg_div_cpu_power(busiest,
				busiest_load_per_task * SCHED_LOAD_SCALE);
		if (max_load > tmp)
			pwr_move += busiest->__cpu_power *
				min(busiest_load_per_task, max_load - tmp);

		/* Amount of load we'd add */
		if (max_load * busiest->__cpu_power <
				busiest_load_per_task * SCHED_LOAD_SCALE)
			tmp = sg_div_cpu_power(this,
					max_load * busiest->__cpu_power);
		else
			tmp = sg_div_cpu_power(this,
				busiest_load_per_task * SCHED_LOAD_SCALE);
		pwr_move += this->__cpu_power *
				min(this_load_per_task, this_load + tmp);
		pwr_move /= SCHED_LOAD_SCALE;

		/* Move if we gain throughput */
		if (pwr_move > pwr_now)
			*imbalance = busiest_load_per_task;
	}

	return busiest;

out_balanced:
ret:
	*imbalance = 0;
	return NULL;
}

/*
 * find_busiest_queue - find the busiest runqueue among the cpus in group.
 */
static struct rq *
find_busiest_queue(struct sched_group *group, enum cpu_idle_type idle,
		   unsigned long imbalance, cpumask_t *cpus)
{
	struct rq *busiest = NULL, *rq;
	unsigned long max_load = 0;
	int i;

	for_each_cpu(i, &group->cpumask) {
		unsigned long wl;

		if (!cpumask_test_cpu(i, cpus))
			continue;

		rq = cpu_rq(i);
		wl = weighted_cpuload(i);

		if (rq->nr_running == 1 && wl > imbalance)
			continue;

		if (wl > max_load) {
			max_load = wl;
			busiest = rq;
		}
	}

	return busiest;
}

/*
 * Max backoff if we encounter pinned tasks. Pretty arbitrary value, but
 * so long as it is large enough.
 */
#define MAX_PINNED_INTERVAL	512

/*
 * Check this_cpu to ensure it is balanced within domain. Attempt to move
 * tasks if there is an imbalance.
 */
static int load_balance(int this_cpu, struct rq *this_rq,
			struct sched_domain *sd, enum cpu_idle_type idle,
			int *balance)
{
	int ld_moved, all_pinned = 0, active_balance = 0, sd_idle = 0;
	struct sched_group *group;
	unsigned long imbalance;
	struct rq *busiest;
	cpumask_t cpus = CPU_MASK_ALL;
	u64 flags;

	/*
	 * When power savings policy is enabled for the parent domain, idle
	 * sibling can pick up load irrespective of busy siblings. In this case,
	 * let the state of idle sibling percolate up as CPU_IDLE, instead of
	 * portraying it as CPU_NOT_IDLE.
	 */
	if (idle != CPU_NOT_IDLE && sd->flags & SD_SHARE_CPUPOWER &&
	    !test_sd_parent(sd, SD_POWERSAVINGS_BALANCE))
		sd_idle = 1;

redo:
	group = find_busiest_group(sd, this_cpu, &imbalance, idle, &sd_idle,
				   &cpus, balance);

	if (*balance == 0)
		goto out_balanced;

	if (!group)
		goto out_balanced;

	busiest = find_busiest_queue(group, idle, imbalance, &cpus);
	if (!busiest)
		goto out_balanced;

	BUG_ON(busiest == this_rq);

	ld_moved = 0;
	if (busiest->nr_running > 1) {
		/*
		 * Attempt to move tasks. If find_busiest_group has found
		 * an imbalance but busiest->nr_running <= 1, the group is
		 * still unbalanced. ld_moved simply stays zero, so it is
		 * correctly treated as an imbalance.
		 */
		local_irq_save(flags);
		double_rq_lock(this_rq, busiest);
		ld_moved = move_tasks(this_rq, this_cpu, busiest,
				      imbalance, sd, idle, &all_pinned);
		double_rq_unlock(this_rq, busiest);
		local_irq_restore(flags);

		/*
		 * some other cpu did the load balance for us.
		 */
		if (ld_moved && this_cpu != smp_processor_id())
			resched_cpu(this_cpu);

		/* All tasks on this runqueue were pinned by CPU affinity */
		if (unlikely(all_pinned)) {
			cpumask_clear_cpu(cpu_of(busiest), &cpus);
			if (!cpumask_empty(&cpus))
				goto redo;
			goto out_balanced;
		}
	}

	if (!ld_moved) {
		sd->nr_balance_failed++;

		if (unlikely(sd->nr_balance_failed > sd->cache_nice_tries+2)) {

			spin_lock_irqsave(&busiest->lock, flags);

			/* don't kick the migration_thread, if the curr
			 * task on busiest cpu can't be moved to this_cpu
			 */
			if (!cpumask_test_cpu(this_cpu, &busiest->curr->cpus_allowed)) {
				spin_unlock_irqrestore(&busiest->lock, flags);
				all_pinned = 1;
				goto out_one_pinned;
			}

			if (!busiest->active_balance) {
				busiest->active_balance = 1;
				busiest->push_cpu = this_cpu;
				active_balance = 1;
			}
			spin_unlock_irqrestore(&busiest->lock, flags);
			if (active_balance)
				wake_up_process(busiest->migration_thread);

			/*
			 * We've kicked active balancing, reset the failure
			 * counter.
			 */
			sd->nr_balance_failed = sd->cache_nice_tries+1;
		}
	} else
		sd->nr_balance_failed = 0;

	if (likely(!active_balance)) {
		/* We were unbalanced, so reset the balancing interval */
		sd->balance_interval = sd->min_interval;
	} else {
		/*
		 * If we've begun active balancing, start to back off. This
		 * case may not be covered by the all_pinned logic if there
		 * is only 1 task on the busy runqueue (because we don't call
		 * move_tasks).
		 */
		if (sd->balance_interval < sd->max_interval)
			sd->balance_interval *= 2;
	}

	if (!ld_moved && !sd_idle && sd->flags & SD_SHARE_CPUPOWER &&
	    !test_sd_parent(sd, SD_POWERSAVINGS_BALANCE))
		return -1;
	return ld_moved;

out_balanced:
	sd->nr_balance_failed = 0;

out_one_pinned:
	/* tune up the balancing interval */
	if ((all_pinned && sd->balance_interval < MAX_PINNED_INTERVAL) ||
			(sd->balance_interval < sd->max_interval))
		sd->balance_interval *= 2;

	if (!sd_idle && sd->flags & SD_SHARE_CPUPOWER &&
	    !test_sd_parent(sd, SD_POWERSAVINGS_BALANCE))
		return -1;
	return 0;
}

/*
 * Check this_cpu to ensure it is balanced within domain. Attempt to move
 * tasks if there is an imbalance.
 *
 * Called from schedule when this_rq is about to become idle (CPU_NEWLY_IDLE).
 * this_rq is locked.
 */
static int
load_balance_newidle(int this_cpu, struct rq *this_rq, struct sched_domain *sd)
{
	struct sched_group *group;
	struct rq *busiest = NULL;
	unsigned long imbalance;
	int ld_moved = 0;
	int sd_idle = 0;
	int all_pinned = 0;
	cpumask_t cpus = CPU_MASK_ALL;

	/*
	 * When power savings policy is enabled for the parent domain, idle
	 * sibling can pick up load irrespective of busy siblings. In this case,
	 * let the state of idle sibling percolate up as IDLE, instead of
	 * portraying it as CPU_NOT_IDLE.
	 */
	if (sd->flags & SD_SHARE_CPUPOWER &&
	    !test_sd_parent(sd, SD_POWERSAVINGS_BALANCE))
		sd_idle = 1;

redo:
	group = find_busiest_group(sd, this_cpu, &imbalance, CPU_NEWLY_IDLE,
				   &sd_idle, &cpus, NULL);
	if (!group)
		goto out_balanced;

	busiest = find_busiest_queue(group, CPU_NEWLY_IDLE, imbalance,
				&cpus);
	if (!busiest)
		goto out_balanced;

	BUG_ON(busiest == this_rq);

	ld_moved = 0;
	if (busiest->nr_running > 1) {
		/* Attempt to move tasks */
		double_lock_balance(this_rq, busiest);
		/* this_rq->clock is already updated */
		update_rq_clock(busiest);
		ld_moved = move_tasks(this_rq, this_cpu, busiest,
					imbalance, sd, CPU_NEWLY_IDLE,
					&all_pinned);
		spin_unlock(&busiest->lock);

		if (unlikely(all_pinned)) {
			cpumask_clear_cpu(cpu_of(busiest), &cpus);
			if (!cpumask_empty(&cpus))
				goto redo;
		}
	}

	if (!ld_moved) {
		if (!sd_idle && sd->flags & SD_SHARE_CPUPOWER &&
		    !test_sd_parent(sd, SD_POWERSAVINGS_BALANCE))
			return -1;
	} else
		sd->nr_balance_failed = 0;

	return ld_moved;

out_balanced:
	if (!sd_idle && sd->flags & SD_SHARE_CPUPOWER &&
	    !test_sd_parent(sd, SD_POWERSAVINGS_BALANCE))
		return -1;
	sd->nr_balance_failed = 0;

	return 0;
}

/*
 * idle_balance is called by schedule() if this_cpu is about to become
 * idle. Attempts to pull tasks from other CPUs.
 */
static void idle_balance(int this_cpu, struct rq *this_rq)
{
	struct sched_domain *sd;
	int pulled_task = -1;
	u64 next_balance = jiffies + HZ;

	for_each_domain(this_cpu, sd) {
		u64 interval;

		if (!(sd->flags & SD_LOAD_BALANCE))
			continue;

		if (sd->flags & SD_BALANCE_NEWIDLE)
			/* If we've pulled tasks over stop searching: */
			pulled_task = load_balance_newidle(this_cpu,
								this_rq, sd);

		interval = msecs_to_jiffies(sd->balance_interval);
		if (time_after(next_balance, sd->last_balance + interval))
			next_balance = sd->last_balance + interval;
		if (pulled_task)
			break;
	}
	if (pulled_task || time_after(jiffies, this_rq->next_balance)) {
		/*
		 * We are going idle. next_balance may be set based on
		 * a busy processor. So reset next_balance.
		 */
		this_rq->next_balance = next_balance;
	}
}

static struct {
	atomic_t load_balancer;
	cpumask_t cpu_mask;
} nohz ____cacheline_aligned = {
	.load_balancer = ATOMIC_INIT(-1),
	.cpu_mask = CPU_MASK_NONE,
};

/*
 * This routine will try to nominate the ilb (idle load balancing)
 * owner among the cpus whose ticks are stopped. ilb owner will do the idle
 * load balancing on behalf of all those cpus. If all the cpus in the system
 * go into this tickless mode, then there will be no ilb owner (as there is
 * no need for one) and all the cpus will sleep till the next wakeup event
 * arrives...
 *
 * For the ilb owner, tick is not stopped. And this tick will be used
 * for idle load balancing. ilb owner will still be part of
 * nohz.cpu_mask..
 *
 * While stopping the tick, this cpu will become the ilb owner if there
 * is no other owner. And will be the owner till that cpu becomes busy
 * or if all cpus in the system stop their ticks at which point
 * there is no need for ilb owner.
 *
 * When the ilb owner becomes busy, it nominates another owner, during the
 * next busy scheduler_tick()
 */
int select_nohz_load_balancer(int stop_tick)
{
	int cpu = smp_processor_id();

	if (stop_tick) {
		cpumask_set_cpu(cpu, &nohz.cpu_mask);
		cpu_rq(cpu)->in_nohz_recently = 1;

		/*
		 * If we are going offline and still the leader, give up!
		 */
		if (cpu_is_offline(cpu) &&
		    atomic_read(&nohz.load_balancer) == cpu) {
			if (atomic_cmpxchg(&nohz.load_balancer, cpu, -1) != cpu)
				BUG();
			return 0;
		}

		/* time for ilb owner also to sleep */
		if (cpumask_weight(&nohz.cpu_mask) == num_online_cpus()) {
			if (atomic_read(&nohz.load_balancer) == cpu)
				atomic_set(&nohz.load_balancer, -1);
			return 0;
		}

		if (atomic_read(&nohz.load_balancer) == -1) {
			/* make me the ilb owner */
			if (atomic_cmpxchg(&nohz.load_balancer, -1, cpu) == -1)
				return 1;
		} else if (atomic_read(&nohz.load_balancer) == cpu)
			return 1;
	} else {
		if (!cpumask_test_cpu(cpu, &nohz.cpu_mask))
			return 0;

		cpumask_clear_cpu(cpu, &nohz.cpu_mask);

		if (atomic_read(&nohz.load_balancer) == cpu)
			if (atomic_cmpxchg(&nohz.load_balancer, cpu, -1) != cpu)
				BUG();
	}
	return 0;
}

static DEFINE_SPINLOCK(balancing);

/*
 * It checks each scheduling domain to see if it is due to be balanced,
 * and initiates a balancing operation if so.
 *
 * Balancing parameters are set up in arch_init_sched_domains.
 */
static void rebalance_domains(int cpu, enum cpu_idle_type idle)
{
	int balance = 1;
	struct rq *rq = cpu_rq(cpu);
	u64 interval;
	struct sched_domain *sd;
	/* Earliest time when we have to do rebalance again */
	u64 next_balance = jiffies + 60*HZ;
	int update_next_balance = 0;

	for_each_domain(cpu, sd) {
		if (!(sd->flags & SD_LOAD_BALANCE))
			continue;

		interval = sd->balance_interval;
		if (idle != CPU_IDLE)
			interval *= sd->busy_factor;

		/* scale ms to jiffies */
		interval = msecs_to_jiffies(interval);
		if (unlikely(!interval))
			interval = 1;
		if (interval > HZ*CONFIG_NR_CPUS/10)
			interval = HZ*CONFIG_NR_CPUS/10;


		if (sd->flags & SD_SERIALIZE) {
			if (!spin_trylock(&balancing))
				goto out;
		}

		if (time_after_eq(jiffies, sd->last_balance + interval)) {
			if (load_balance(cpu, rq, sd, idle, &balance)) {
				/*
				 * We've pulled tasks over so either we're no
				 * longer idle, or one of our SMT siblings is
				 * not idle.
				 */
				idle = CPU_NOT_IDLE;
			}
			sd->last_balance = jiffies;
		}
		if (sd->flags & SD_SERIALIZE)
			spin_unlock(&balancing);
out:
		if (time_after(next_balance, sd->last_balance + interval)) {
			next_balance = sd->last_balance + interval;
			update_next_balance = 1;
		}

		/*
		 * Stop the load balance at this level. There is another
		 * CPU in our sched group which is doing load balancing more
		 * actively.
		 */
		if (!balance)
			break;
	}

	/*
	 * next_balance will be updated only when there is a need.
	 * When the cpu is attached to null domain for ex, it will not be
	 * updated.
	 */
	if (likely(update_next_balance))
		rq->next_balance = next_balance;
}

/*
 * run_rebalance_domains is triggered when needed from the scheduler tick.
 * In CONFIG_NO_HZ case, the idle load balance owner will do the
 * rebalancing for all the cpus for whom scheduler ticks are stopped.
 */
static void run_rebalance_domains(struct softirq_action *h)
{
	int this_cpu = smp_processor_id();
	struct rq *this_rq = cpu_rq(this_cpu);
	enum cpu_idle_type idle = this_rq->idle_at_tick ?
						CPU_IDLE : CPU_NOT_IDLE;

	rebalance_domains(this_cpu, idle);

	/*
	 * If this cpu is the owner for idle load balancing, then do the
	 * balancing on behalf of the other idle cpus whose ticks are
	 * stopped.
	 */
	if (this_rq->idle_at_tick &&
	    atomic_read(&nohz.load_balancer) == this_cpu) {
		cpumask_t cpus = nohz.cpu_mask;
		struct rq *rq;
		int balance_cpu;

		cpumask_clear_cpu(this_cpu, &cpus);
		for_each_cpu(balance_cpu, &cpus) {
			/*
			 * If this cpu gets work to do, stop the load balancing
			 * work being done for other cpus. Next load
			 * balancing owner will pick it up.
			 */
			if (need_resched())
				break;

			rebalance_domains(balance_cpu, CPU_IDLE);

			rq = cpu_rq(balance_cpu);
			if (time_after(this_rq->next_balance, rq->next_balance))
				this_rq->next_balance = rq->next_balance;
		}
	}
}

/*
 * Trigger the SCHED_SOFTIRQ if it is time to do periodic load balancing.
 *
 * In case of CONFIG_NO_HZ, this is the place where we nominate a new
 * idle load balancing owner or decide to stop the periodic load balancing,
 * if the whole system is idle.
 */
static inline void trigger_load_balance(struct rq *rq, int cpu)
{
	/*
	 * If we were in the nohz mode recently and busy at the current
	 * scheduler tick, then check if we need to nominate new idle
	 * load balancer.
	 */
	if (rq->in_nohz_recently && !rq->idle_at_tick) {
		rq->in_nohz_recently = 0;

		if (atomic_read(&nohz.load_balancer) == cpu) {
			cpumask_clear_cpu(cpu, &nohz.cpu_mask);
			atomic_set(&nohz.load_balancer, -1);
		}

		if (atomic_read(&nohz.load_balancer) == -1) {
			/*
			 * simple selection for now: Nominate the
			 * first cpu in the nohz list to be the next
			 * ilb owner.
			 *
			 * TBD: Traverse the sched domains and nominate
			 * the nearest cpu in the nohz.cpu_mask.
			 */
			int ilb = cpumask_first(&nohz.cpu_mask);

			if (ilb != CONFIG_NR_CPUS)
				resched_cpu(ilb);
		}
	}

	/*
	 * If this cpu is idle and doing idle load balancing for all the
	 * cpus with ticks stopped, is it time for that to stop?
	 */
	if (rq->idle_at_tick && atomic_read(&nohz.load_balancer) == cpu &&
	    cpumask_weight(&nohz.cpu_mask) == num_online_cpus()) {
		resched_cpu(cpu);
		return;
	}

	/*
	 * If this cpu is idle and the idle load balancing is done by
	 * someone else, then no need raise the SCHED_SOFTIRQ
	 */
	if (rq->idle_at_tick && atomic_read(&nohz.load_balancer) != cpu &&
	    cpumask_test_cpu(cpu, &nohz.cpu_mask))
		return;

	if (time_after_eq(jiffies, rq->next_balance))
		raise_softirq(SCHED_SOFTIRQ);
}

/*
 * Return p->sum_exec_runtime plus any more ns on the sched_clock
 * that have not yet been banked in case the task is currently running.
 */
unsigned long long task_sched_runtime(struct task_struct *p)
{
	u64 flags;
	u64 ns, delta_exec;
	struct rq *rq;

	rq = task_rq_lock(p, &flags);
	ns = p->se.sum_exec_runtime;
	if (task_current(rq, p)) {
		update_rq_clock(rq);
		delta_exec = rq->clock - p->se.exec_start;
		if ((s64)delta_exec > 0)
			ns += delta_exec;
	}
	task_rq_unlock(rq, &flags);

	return ns;
}

/*
 * This function gets called by the timer code, with HZ frequency.
 * We call it with interrupts disabled.
 *
 * It also gets called by the fork code, when changing the parent's
 * timeslices.
 */
void scheduler_tick(void)
{
	int cpu = smp_processor_id();
	struct rq *rq = cpu_rq(cpu);
	struct task_struct *curr = rq->curr;
	u64 next_tick = rq->tick_timestamp + TICK_NSEC;

	spin_lock(&rq->lock);
	__update_rq_clock(rq);
	/*
	 * Let rq->clock advance by at least TICK_NSEC:
	 */
	if (unlikely(rq->clock < next_tick))
		rq->clock = next_tick;
	rq->tick_timestamp = rq->clock;

	update_cpu_load(rq);
	if (curr != rq->idle) /* FIXME: needed? */
		curr->sched_class->task_tick(rq, curr);
	spin_unlock(&rq->lock);

	rq->idle_at_tick = idle_cpu(cpu);
	trigger_load_balance(rq, cpu);
}

/*
 * Print scheduling while atomic bug:
 */
static noinline void __schedule_bug(struct task_struct *prev)
{
	struct pt_regs *regs = get_irq_regs();

	printf("BUG: scheduling while atomic: %s/%d/0x%08x\n",
		prev->comm, prev->pid.pid, preempt_count());

	if (irqs_disabled())
		print_irqtrace_events(prev);

	if (regs)
		show_regs(regs);
	else
		dump_stack();
}

/*
 * Various schedule()-time debugging checks and statistics:
 */
static inline void schedule_debug(struct task_struct *prev)
{
	/*
	 * Test if we are atomic. Since do_exit() needs to call into
	 * schedule() atomically, we ignore that path for now.
	 * Otherwise, whine if we are scheduling when we should not be.
	 */
	if (unlikely(in_atomic_preempt_off()) && unlikely(!prev->exit_state))
		__schedule_bug(prev);
}

/*
 * Pick up the highest-prio task:
 */
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev)
{
	const struct sched_class *class;
	struct task_struct *p;

	/*
	 * Optimization: we know that if all tasks are in
	 * the fair class we can call that function directly:
	 */
	if (likely(rq->nr_running == rq->cfs.nr_running)) {
		p = fair_sched_class.pick_next_task(rq);
		if (likely(p))
			return p;
	}

	class = sched_class_highest;
	for ( ; ; ) {
		p = class->pick_next_task(rq);
		if (p)
			return p;
		/*
		 * Will never be NULL as the idle class always
		 * returns a non-NULL p:
		 */
		class = class->next;
	}
}

/*
 * schedule() is the main scheduler function.
 */
asmlinkage void __sched schedule(void)
{
	struct task_struct *prev, *next;
	long *switch_count;
	struct rq *rq;
	int cpu;

need_resched:
	preempt_disable();
	cpu = smp_processor_id();
	rq = cpu_rq(cpu);
	prev = rq->curr;
	switch_count = &prev->nivcsw;

	schedule_debug(prev);

	/*
	 * Do the rq-clock update outside the rq lock:
	 */
	local_irq_disable();
	__update_rq_clock(rq);
	spin_lock(&rq->lock);
	clear_tsk_need_resched(prev);

	if (prev->state && !(preempt_count() & PREEMPT_ACTIVE)) {
		if (unlikely((prev->state & TASK_INTERRUPTIBLE) &&
				unlikely(signal_pending(prev)))) {
			prev->state = TASK_RUNNING;
		} else {
			deactivate_task(rq, prev, 1);
		}
		switch_count = &prev->nvcsw;
	}

	if (unlikely(!rq->nr_running))
		idle_balance(cpu, rq);

	prev->sched_class->put_prev_task(rq, prev);
	next = pick_next_task(rq, prev);

	if (likely(prev != next)) {
		rq->nr_switches++;
		rq->curr = next;
		++*switch_count;

		context_switch(rq, prev, next); /* unlocks the rq */
	} else
		spin_unlock_irq(&rq->lock);

	preempt_enable_no_resched();
	if (unlikely(test_thread_flag(TIF_NEED_RESCHED)))
		goto need_resched;
}

/*
 * this is the entry point to schedule() from in-kernel preemption
 * off of preempt_enable. Kernel preemptions off return from interrupt
 * occur there and call schedule directly.
 */
asmlinkage void __sched preempt_schedule(void)
{
	/*
	 * If there is a non-zero preempt_count or interrupts are disabled,
	 * we do not want to preempt the current task. Just return..
	 */
	if (likely(!preemptible()))
		return;

	do {
		__preempt_count_add(PREEMPT_ACTIVE);

		/*
		 * We keep the big kernel semaphore locked, but we
		 * clear ->lock_depth so that schedule() doesnt
		 * auto-release the semaphore:
		 */
		schedule();

		__preempt_count_sub(PREEMPT_ACTIVE);

		/*
		 * Check again in case we missed a preemption opportunity
		 * between schedule and now.
		 */
		barrier();
	} while (need_resched());
}

/*
 * this is the entry point to schedule() from kernel preemption
 * off of irq context.
 * Note, that this is called and return with irqs disabled. This will
 * protect us against recursive calling from irq.
 */
asmlinkage void __sched preempt_schedule_irq(void)
{
	/* Catch callers which need to be fixed */
	BUG_ON(preempt_count() || !irqs_disabled());

	do {
		__preempt_count_add(PREEMPT_ACTIVE);

		/*
		 * We keep the big kernel semaphore locked, but we
		 * clear ->lock_depth so that schedule() doesnt
		 * auto-release the semaphore:
		 */
		local_irq_enable();
		schedule();
		local_irq_disable();

		__preempt_count_sub(PREEMPT_ACTIVE);

		/*
		 * Check again in case we missed a preemption opportunity
		 * between schedule and now.
		 */
		barrier();
	} while (need_resched());
}

int default_wake_function(wait_queue_t *curr, unsigned mode, int sync,
			  void *key)
{
	return try_to_wake_up(curr->private, mode, sync);
}

/*
 * The core wakeup function. Non-exclusive wakeups (nr_exclusive == 0) just
 * wake everything up. If it's an exclusive wakeup (nr_exclusive == small +ve
 * number) then we wake all the non-exclusive tasks and one exclusive task.
 *
 * There are circumstances in which we can try to wake a task which has already
 * started to run but is not in state TASK_RUNNING. try_to_wake_up() returns
 * zero in this (rare) case, and we handle it by continuing to scan the queue.
 */
static void __wake_up_common(wait_queue_head_t *q, unsigned int mode,
			     int nr_exclusive, int sync, void *key)
{
	wait_queue_t *curr, *next;

	list_for_each_entry_safe(curr, next, &q->task_list, task_list) {
		unsigned flags = curr->flags;

		if (curr->func(curr, mode, sync, key) &&
				(flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
			break;
	}
}

/**
 * __wake_up - wake up threads blocked on a waitqueue.
 * @q: the waitqueue
 * @mode: which threads
 * @nr_exclusive: how many wake-one or wake-many threads to wake up
 * @key: is directly passed to the wakeup function
 */
void __wake_up(wait_queue_head_t *q, unsigned int mode,
			int nr_exclusive, void *key)
{
	u64 flags;

	spin_lock_irqsave(&q->lock, flags);
	__wake_up_common(q, mode, nr_exclusive, 0, key);
	spin_unlock_irqrestore(&q->lock, flags);
}

/*
 * Same as __wake_up but called with the spinlock in wait_queue_head_t held.
 */
void __wake_up_locked(wait_queue_head_t *q, unsigned int mode)
{
	__wake_up_common(q, mode, 1, 0, NULL);
}

/**
 * __wake_up_sync - wake up threads blocked on a waitqueue.
 * @q: the waitqueue
 * @mode: which threads
 * @nr_exclusive: how many wake-one or wake-many threads to wake up
 *
 * The sync wakeup differs that the waker knows that it will schedule
 * away soon, so while the target thread will be woken up, it will not
 * be migrated to another CPU - ie. the two threads are 'synchronized'
 * with each other. This can prevent needless bouncing between CPUs.
 *
 * On UP it can prevent extra preemption.
 */
void
__wake_up_sync(wait_queue_head_t *q, unsigned int mode, int nr_exclusive)
{
	u64 flags;
	int sync = 1;

	if (unlikely(!q))
		return;

	if (unlikely(!nr_exclusive))
		sync = 0;

	spin_lock_irqsave(&q->lock, flags);
	__wake_up_common(q, mode, nr_exclusive, sync, NULL);
	spin_unlock_irqrestore(&q->lock, flags);
}

void complete(struct completion *x)
{
	u64 flags;

	spin_lock_irqsave(&x->wait.lock, flags);
	x->done++;
	__wake_up_common(&x->wait, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE,
			 1, 0, NULL);
	spin_unlock_irqrestore(&x->wait.lock, flags);
}

void complete_all(struct completion *x)
{
	u64 flags;

	spin_lock_irqsave(&x->wait.lock, flags);
	x->done += UINT_MAX/2;
	__wake_up_common(&x->wait, TASK_UNINTERRUPTIBLE | TASK_INTERRUPTIBLE,
			 0, 0, NULL);
	spin_unlock_irqrestore(&x->wait.lock, flags);
}

static inline long __sched
do_wait_for_common(struct completion *x, long timeout, int state)
{
	if (!x->done) {
		DECLARE_WAITQUEUE(wait, current);

		wait.flags |= WQ_FLAG_EXCLUSIVE;
		__add_wait_queue_tail(&x->wait, &wait);
		do {
			if (state == TASK_INTERRUPTIBLE &&
			    signal_pending(current)) {
				__remove_wait_queue(&x->wait, &wait);
				return -ERESTARTSYS;
			}
			__set_current_state(state);
			spin_unlock_irq(&x->wait.lock);
			timeout = schedule_timeout(timeout);
			spin_lock_irq(&x->wait.lock);
			if (!timeout) {
				__remove_wait_queue(&x->wait, &wait);
				return timeout;
			}
		} while (!x->done);
		__remove_wait_queue(&x->wait, &wait);
	}
	x->done--;
	return timeout;
}

static long __sched
wait_for_common(struct completion *x, long timeout, int state)
{
	spin_lock_irq(&x->wait.lock);
	timeout = do_wait_for_common(x, timeout, state);
	spin_unlock_irq(&x->wait.lock);
	return timeout;
}

void __sched wait_for_completion(struct completion *x)
{
	wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_UNINTERRUPTIBLE);
}

unsigned long __sched
wait_for_completion_timeout(struct completion *x, unsigned long timeout)
{
	return wait_for_common(x, timeout, TASK_UNINTERRUPTIBLE);
}

int __sched wait_for_completion_interruptible(struct completion *x)
{
	long t = wait_for_common(x, MAX_SCHEDULE_TIMEOUT, TASK_INTERRUPTIBLE);
	if (t == -ERESTARTSYS)
		return t;
	return 0;
}

unsigned long __sched
wait_for_completion_interruptible_timeout(struct completion *x,
					  unsigned long timeout)
{
	return wait_for_common(x, timeout, TASK_INTERRUPTIBLE);
}

static long __sched
sleep_on_common(wait_queue_head_t *q, int state, long timeout)
{
	u64 flags;
	wait_queue_t wait;

	init_waitqueue_entry(&wait, current);

	__set_current_state(state);

	spin_lock_irqsave(&q->lock, flags);
	__add_wait_queue(q, &wait);
	spin_unlock(&q->lock);
	timeout = schedule_timeout(timeout);
	spin_lock_irq(&q->lock);
	__remove_wait_queue(q, &wait);
	spin_unlock_irqrestore(&q->lock, flags);

	return timeout;
}

void __sched interruptible_sleep_on(wait_queue_head_t *q)
{
	sleep_on_common(q, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}

long __sched
interruptible_sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
	return sleep_on_common(q, TASK_INTERRUPTIBLE, timeout);
}

void __sched sleep_on(wait_queue_head_t *q)
{
	sleep_on_common(q, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}

long __sched sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
	return sleep_on_common(q, TASK_UNINTERRUPTIBLE, timeout);
}

void set_user_nice(struct task_struct *p, long nice)
{
	int old_prio, delta, on_rq;
	u64 flags;
	struct rq *rq;

	if (TASK_NICE(p) == nice || nice < -20 || nice > 19)
		return;
	/*
	 * We have to be careful, if called from sys_setpriority(),
	 * the task might be in the middle of scheduling on another CPU.
	 */
	rq = task_rq_lock(p, &flags);
	update_rq_clock(rq);
	/*
	 * The RT priorities are set via sched_setscheduler(), but we still
	 * allow the 'normal' nice value to be set - but as expected
	 * it wont have any effect on scheduling until the task is
	 * SCHED_FIFO/SCHED_RR:
	 */
	if (task_has_rt_policy(p)) {
		p->static_prio = NICE_TO_PRIO(nice);
		goto out_unlock;
	}
	on_rq = p->se.on_rq;
	if (on_rq) {
		dequeue_task(rq, p, 0);
		dec_load(rq, p);
	}

	p->static_prio = NICE_TO_PRIO(nice);
	set_load_weight(p);
	old_prio = p->prio;
	p->prio = effective_prio(p);
	delta = p->prio - old_prio;

	if (on_rq) {
		enqueue_task(rq, p, 0);
		inc_load(rq, p);
		/*
		 * If the task increased its priority or is running and
		 * lowered its priority, then reschedule its CPU:
		 */
		if (delta < 0 || (delta > 0 && task_running(rq, p)))
			resched_task(rq->curr);
	}
out_unlock:
	task_rq_unlock(rq, &flags);
}

/**
 * task_prio - return the priority value of a given task.
 * @p: the task in question.
 *
 * This is the priority value as seen by users in /proc.
 * RT tasks are offset by -200. Normal tasks are centered
 * around 0, value goes from -16 to +15.
 */
int task_prio(const struct task_struct *p)
{
	return p->prio - MAX_RT_PRIO;
}

/**
 * task_nice - return the nice value of a given task.
 * @p: the task in question.
 */
int task_nice(const struct task_struct *p)
{
	return TASK_NICE(p);
}

/* Actually do priority change: must hold rq lock. */
static void
__setscheduler(struct rq *rq, struct task_struct *p, int policy, int prio)
{
	BUG_ON(p->se.on_rq);

	p->policy = policy;
	switch (p->policy) {
	case SCHED_NORMAL:
	case SCHED_BATCH:
	case SCHED_IDLE:
		p->sched_class = &fair_sched_class;
		break;
	case SCHED_FIFO:
	case SCHED_RR:
		p->sched_class = &rt_sched_class;
		break;
	}

	p->rt_priority = prio;
	p->normal_prio = normal_prio(p);
	/* we are holding p->pi_lock already */
	p->prio = p->normal_prio;
	set_load_weight(p);
}

/**
 * sched_setscheduler - change the scheduling policy and/or RT priority of a thread.
 * @p: the task in question.
 * @policy: new policy.
 * @param: structure containing the new RT priority.
 *
 * NOTE that the task may be already dead.
 */
int sched_setscheduler(struct task_struct *p, int policy,
		       struct sched_param *param)
{
	int oldprio, oldpolicy = -1, on_rq, running;
	u64 flags;
	struct rq *rq;

	/* may grab non-irq protected spin_locks */
	BUG_ON(in_interrupt());
recheck:
	/* double check policy once rq lock held */
	if (policy < 0)
		policy = oldpolicy = p->policy;
	else if (policy != SCHED_FIFO && policy != SCHED_RR &&
			policy != SCHED_NORMAL && policy != SCHED_BATCH &&
			policy != SCHED_IDLE)
		return -EINVAL;
	/*
	 * Valid priorities for SCHED_FIFO and SCHED_RR are
	 * 1..MAX_USER_RT_PRIO-1, valid priority for SCHED_NORMAL,
	 * SCHED_BATCH and SCHED_IDLE is 0.
	 */
	if (param->sched_priority < 0 ||
	    (p->mm && param->sched_priority > MAX_USER_RT_PRIO-1) ||
	    (!p->mm && param->sched_priority > MAX_RT_PRIO-1))
		return -EINVAL;
	if (rt_policy(policy) != (param->sched_priority != 0))
		return -EINVAL;

	/*
	 * make sure no PI-waiters arrive (or leave) while we are
	 * changing the priority of the task:
	 */
	spin_lock_irqsave(&p->pi_lock, flags);
	/*
	 * To be able to change p->policy safely, the apropriate
	 * runqueue lock must be held.
	 */
	rq = __task_rq_lock(p);
	/* recheck policy now with rq lock held */
	if (unlikely(oldpolicy != -1 && oldpolicy != p->policy)) {
		policy = oldpolicy = -1;
		__task_rq_unlock(rq);
		spin_unlock_irqrestore(&p->pi_lock, flags);
		goto recheck;
	}
	update_rq_clock(rq);
	on_rq = p->se.on_rq;
	running = task_current(rq, p);
	if (on_rq) {
		deactivate_task(rq, p, 0);
		if (running)
			p->sched_class->put_prev_task(rq, p);
	}

	oldprio = p->prio;
	__setscheduler(rq, p, policy, param->sched_priority);

	if (on_rq) {
		if (running)
			p->sched_class->set_curr_task(rq);
		activate_task(rq, p, 0);
		/*
		 * Reschedule if we are currently running on this runqueue and
		 * our priority decreased, or if we are not currently running on
		 * this runqueue and our priority is higher than the current's
		 */
		if (running) {
			if (p->prio > oldprio)
				resched_task(rq->curr);
		} else {
			check_preempt_curr(rq, p);
		}
	}
	__task_rq_unlock(rq);
	spin_unlock_irqrestore(&p->pi_lock, flags);

	return 0;
}

long sched_setaffinity(pid_t pid, cpumask_t new_mask)
{
	cpumask_t cpus_allowed;
	struct task_struct *p;
	int retval;

	mutex_lock(&sched_hotcpu_mutex);
	read_lock(&tasklist_lock);

	p = find_process_by_pid(pid);
	if (!p) {
		read_unlock(&tasklist_lock);
		mutex_unlock(&sched_hotcpu_mutex);
		return -ESRCH;
	}

	/*
	 * It is not safe to call set_cpus_allowed with the
	 * tasklist_lock held. We will bump the task_struct's
	 * usage count and then drop tasklist_lock.
	 */
	get_task_struct(p);
	read_unlock(&tasklist_lock);

	cpus_allowed = cpuset_cpus_allowed(p);
	cpumask_and(&new_mask, &new_mask, &cpus_allowed);
 again:
	retval = set_cpus_allowed(p, new_mask);

	if (!retval) {
		cpus_allowed = cpuset_cpus_allowed(p);
		if (!cpumask_subset(&new_mask, &cpus_allowed)) {
			/*
			 * We must have raced with a concurrent cpuset
			 * update. Just reset the cpus_allowed to the
			 * cpuset's cpus_allowed
			 */
			new_mask = cpus_allowed;
			goto again;
		}
	}

	put_task_struct(p);
	mutex_unlock(&sched_hotcpu_mutex);
	return retval;
}

long sched_getaffinity(pid_t pid, cpumask_t *mask)
{
	struct task_struct *p;
	int retval;

	mutex_lock(&sched_hotcpu_mutex);
	read_lock(&tasklist_lock);

	retval = -ESRCH;
	p = find_process_by_pid(pid);
	if (!p)
		goto out_unlock;

	cpumask_and(mask, &p->cpus_allowed, cpu_online_mask);

out_unlock:
	read_unlock(&tasklist_lock);
	mutex_unlock(&sched_hotcpu_mutex);

	return retval;
}

/**
 * sys_sched_yield - yield the current processor to other threads.
 *
 * This function yields the current CPU to other tasks. If there are no
 * other threads running on this CPU then this function will return.
 */
asmlinkage long sys_sched_yield(void)
{
	struct rq *rq = this_rq_lock();

	current->sched_class->yield_task(rq);

	/*
	 * Since we are going to call schedule() anyway, there's
	 * no need to preempt or enable interrupts:
	 */
	__release(rq->lock);
	spin_release(&rq->lock.dep_map, 1, _THIS_IP_);
	spin_unlock(&rq->lock);
	preempt_enable_no_resched();

	schedule();

	return 0;
}

/**
 * yield - yield the current processor to other threads.
 *
 * This is a shortcut for kernel-space yielding - it marks the
 * thread runnable and calls sys_sched_yield().
 */
void __sched yield(void)
{
	set_current_state(TASK_RUNNING);
	sys_sched_yield();
}

/*
 * This task is about to go to sleep on IO. Increment rq->nr_iowait so
 * that process accounting knows that this is a task in IO wait state.
 *
 * But don't do that if it is a deliberate, throttling IO wait (this task
 * has set its backing_dev_info: the queue against which it should throttle)
 */
void __sched io_schedule(void)
{
	struct rq *rq = cpu_rq(smp_processor_id());

	atomic_inc(&rq->nr_iowait);
	schedule();
	atomic_dec(&rq->nr_iowait);
}

long __sched io_schedule_timeout(long timeout)
{
	struct rq *rq = cpu_rq(smp_processor_id());
	long ret;

	atomic_inc(&rq->nr_iowait);
	ret = schedule_timeout(timeout);
	atomic_dec(&rq->nr_iowait);

	return ret;
}

/**
 * sys_sched_get_priority_max - return maximum RT priority.
 * @policy: scheduling class.
 *
 * this syscall returns the maximum rt_priority that can be used
 * by a given scheduling class.
 */
asmlinkage long sys_sched_get_priority_max(int policy)
{
	int ret = -EINVAL;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		ret = MAX_USER_RT_PRIO-1;
		break;
	case SCHED_NORMAL:
	case SCHED_BATCH:
	case SCHED_IDLE:
		ret = 0;
		break;
	}
	return ret;
}

/**
 * sys_sched_get_priority_min - return minimum RT priority.
 * @policy: scheduling class.
 *
 * this syscall returns the minimum rt_priority that can be used
 * by a given scheduling class.
 */
asmlinkage long sys_sched_get_priority_min(int policy)
{
	int ret = -EINVAL;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		ret = 1;
		break;
	case SCHED_NORMAL:
	case SCHED_BATCH:
	case SCHED_IDLE:
		ret = 0;
	}
	return ret;
}

static const char stat_nam[] = "RSDTtZX";

void show_task(struct task_struct *p)
{
	unsigned long free = 0;
	unsigned state;

	state = p->state ? __ffs(p->state) + 1 : 0;
	printf("%-13.13s %c", p->comm,
		state < sizeof(stat_nam) - 1 ? stat_nam[state] : '?');
#if BITS_PER_LONG == 32
	if (state == TASK_RUNNING)
		printf(KERN_CONT " running  ");
	else
		printf(KERN_CONT " %08lx ", thread_saved_pc(p));
#else
	if (state == TASK_RUNNING)
		printf(KERN_CONT "  running task    ");
	else
		printf(KERN_CONT " %016llx ", thread_saved_pc(p));
#endif
	printf(KERN_CONT "%5lu %5d %6d\n", free,
		task_pid_nr(p), p->real_parent ? task_pid_nr(p->real_parent) : 0);

	if (state != TASK_RUNNING)
		show_stack(p, NULL);
}

/**
 * init_idle - set up an idle thread for a given CPU
 * @idle: task in question
 * @cpu: cpu the idle task belongs to
 *
 * NOTE: this function does not set the idle thread's NEED_RESCHED
 * flag, to make booting more robust.
 */
void init_idle(struct task_struct *idle, int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	u64 flags;

	__sched_fork(idle);
	idle->se.exec_start = sched_clock();

	idle->prio = idle->normal_prio = MAX_PRIO;
	idle->cpus_allowed = *cpumask_of(cpu);
	__set_task_cpu(idle, cpu);

	spin_lock_irqsave(&rq->lock, flags);
	rq->curr = rq->idle = idle;
#if defined(__ARCH_WANT_UNLOCKED_CTXSW)
	idle->oncpu = 1;
#endif
	spin_unlock_irqrestore(&rq->lock, flags);

	task_thread_info(idle)->preempt_count = 0;

	/*
	 * The idle tasks have their own, simple scheduling class:
	 */
	idle->sched_class = &idle_sched_class;
}

/*
 * In a system that switches off the HZ timer nohz_cpu_mask
 * indicates which cpus entered this state. This is used
 * in the rcu update to wait only for active cpus. For system
 * which do not switch off the HZ timer nohz_cpu_mask should
 * always be CPU_MASK_NONE.
 */
cpumask_t nohz_cpu_mask = CPU_MASK_NONE;

/*
 * Increase the granularity value when there are more CPUs,
 * because with more CPUs the 'effective latency' as visible
 * to users decreases. But the relationship is not linear,
 * so pick a second-best guess by going with the log2 of the
 * number of CPUs.
 *
 * This idea comes from the SD scheduler of Con Kolivas:
 */
static inline void sched_init_granularity(void)
{
	unsigned int factor = 1 + ilog2(num_online_cpus());
	const unsigned long limit = 200000000;

	sysctl_sched_min_granularity *= factor;
	if (sysctl_sched_min_granularity > limit)
		sysctl_sched_min_granularity = limit;

	sysctl_sched_latency *= factor;
	if (sysctl_sched_latency > limit)
		sysctl_sched_latency = limit;

	sysctl_sched_wakeup_granularity *= factor;
	sysctl_sched_batch_wakeup_granularity *= factor;
}

/*
 * This is how migration works:
 *
 * 1) we queue a struct migration_req structure in the source CPU's
 *    runqueue and wake up that CPU's migration thread.
 * 2) we down() the locked semaphore => thread blocks.
 * 3) migration thread wakes up (implicitly it forces the migrated
 *    thread off the CPU)
 * 4) it gets the migration request and checks whether the migrated
 *    task is still in the wrong runqueue.
 * 5) if it's in the wrong runqueue then the migration thread removes
 *    it and puts it into the right queue.
 * 6) migration thread up()s the semaphore.
 * 7) we wake up and the migration is done.
 */

/*
 * Change a given task's CPU affinity. Migrate the thread to a
 * proper CPU and schedule it away if the CPU it's executing on
 * is removed from the allowed bitmask.
 *
 * NOTE: the caller must have a valid reference to the task, the
 * task must not exit() & deallocate itself prematurely. The
 * call is not atomic; no spinlocks may be held.
 */
int set_cpus_allowed(struct task_struct *p, cpumask_t new_mask)
{
	struct migration_req req;
	u64 flags;
	struct rq *rq;
	int ret = 0;

	rq = task_rq_lock(p, &flags);
	if (!cpumask_intersects(&new_mask, cpu_online_mask)) {
		ret = -EINVAL;
		goto out;
	}

	p->cpus_allowed = new_mask;
	/* Can the task run on the task's current CPU? If so, we're done */
	if (cpumask_test_cpu(task_cpu(p), &new_mask))
		goto out;

	if (migrate_task(p, cpumask_any_online(&new_mask), &req)) {
		/* Need help from migration thread: drop lock and wait. */
		task_rq_unlock(rq, &flags);
		wake_up_process(rq->migration_thread);
		wait_for_completion(&req.done);

		return 0;
	}
out:
	task_rq_unlock(rq, &flags);

	return ret;
}

static int sd_degenerate(struct sched_domain *sd)
{
	if (cpumask_weight(&sd->span) == 1)
		return 1;

	/* Following flags need at least 2 groups */
	if (sd->flags & (SD_LOAD_BALANCE |
			 SD_BALANCE_NEWIDLE |
			 SD_BALANCE_FORK |
			 SD_BALANCE_EXEC |
			 SD_SHARE_CPUPOWER |
			 SD_SHARE_PKG_RESOURCES)) {
		if (sd->groups != sd->groups->next)
			return 0;
	}

	/* Following flags don't use groups */
	if (sd->flags & (SD_WAKE_IDLE |
			 SD_WAKE_AFFINE |
			 SD_WAKE_BALANCE))
		return 0;

	return 1;
}

static int
sd_parent_degenerate(struct sched_domain *sd, struct sched_domain *parent)
{
	unsigned long cflags = sd->flags, pflags = parent->flags;

	if (sd_degenerate(parent))
		return 1;

	if (!cpumask_equal(&sd->span, &parent->span))
		return 0;

	/* Does parent contain flags not in child? */
	/* WAKE_BALANCE is a subset of WAKE_AFFINE */
	if (cflags & SD_WAKE_AFFINE)
		pflags &= ~SD_WAKE_BALANCE;
	/* Flags needing groups don't count if only 1 group in parent */
	if (parent->groups == parent->groups->next) {
		pflags &= ~(SD_LOAD_BALANCE |
				SD_BALANCE_NEWIDLE |
				SD_BALANCE_FORK |
				SD_BALANCE_EXEC |
				SD_SHARE_CPUPOWER |
				SD_SHARE_PKG_RESOURCES);
	}
	if (~cflags & pflags)
		return 0;

	return 1;
}

/*
 * Attach the domain 'sd' to 'cpu' as its base domain.  Callers must
 * hold the hotplug lock.
 */
static void cpu_attach_domain(struct sched_domain *sd, int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	struct sched_domain *tmp;

	/* Remove the sched domains which do not contribute to scheduling. */
	for (tmp = sd; tmp; tmp = tmp->parent) {
		struct sched_domain *parent = tmp->parent;
		if (!parent)
			break;
		if (sd_parent_degenerate(tmp, parent)) {
			tmp->parent = parent->parent;
			if (parent->parent)
				parent->parent->child = tmp;
		}
	}

	if (sd && sd_degenerate(sd)) {
		sd = sd->parent;
		if (sd)
			sd->child = NULL;
	}

	smp_wmb();
	rq->sd = sd;
}

/* cpus with isolated domains */
static cpumask_t cpu_isolated_map = CPU_MASK_NONE;

/*
 * init_sched_build_groups takes the cpumask we wish to span, and a pointer
 * to a function which identifies what group(along with sched group) a CPU
 * belongs to. The return value of group_fn must be a >= 0 and < NR_CPUS
 * (due to the fact that we keep track of groups covered with a cpumask_t).
 *
 * init_sched_build_groups will build a circular linked list of the groups
 * covered by the given span, and will set each group's ->cpumask correctly,
 * and ->cpu_power to 0.
 */
static void
init_sched_build_groups(cpumask_t span, const cpumask_t *cpu_map,
			int (*group_fn)(int cpu, const cpumask_t *cpu_map,
					struct sched_group **sg))
{
	struct sched_group *first = NULL, *last = NULL;
	cpumask_t covered = CPU_MASK_NONE;
	int i;

	for_each_cpu(i, &span) {
		struct sched_group *sg;
		int group = group_fn(i, cpu_map, &sg);
		int j;

		if (cpumask_test_cpu(i, &covered))
			continue;

		sg->cpumask = CPU_MASK_NONE;
		sg->__cpu_power = 0;

		for_each_cpu(j, &span) {
			if (group_fn(j, cpu_map, NULL) != group)
				continue;

			cpumask_set_cpu(j, &covered);
			cpumask_set_cpu(j, &sg->cpumask);
		}
		if (!first)
			first = sg;
		if (last)
			last->next = sg;
		last = sg;
	}
	last->next = first;
}

static struct sched_domain phys_domains[CONFIG_NR_CPUS];
static struct sched_group sched_group_phys[CONFIG_NR_CPUS];

static int
cpu_to_phys_group(int cpu, const cpumask_t *cpu_map, struct sched_group **sg)
{
	int group;

	group = cpu;

	if (sg) 
		*sg = &sched_group_phys[group];
	return group;
}


/*
 * Initialize sched groups cpu_power.
 *
 * cpu_power indicates the capacity of sched group, which is used while
 * distributing the load between different sched groups in a sched domain.
 * Typically cpu_power for all the groups in a sched domain will be same unless
 * there are asymmetries in the topology. If there are asymmetries, group
 * having more cpu_power will pickup more load compared to the group having
 * less cpu_power.
 *
 * cpu_power will be a multiple of SCHED_LOAD_SCALE. This multiple represents
 * the maximum number of tasks a group can handle in the presence of other idle
 * or lightly loaded groups in the same sched domain.
 */
static void init_sched_groups_power(int cpu, struct sched_domain *sd)
{
	struct sched_domain *child;
	struct sched_group *group;

	WARN_ON(!sd || !sd->groups);

	if (cpu != cpumask_first(&sd->groups->cpumask))
		return;

	child = sd->child;

	sd->groups->__cpu_power = 0;

	/*
	 * For perf policy, if the groups in child domain share resources
	 * (for example cores sharing some portions of the cache hierarchy
	 * or SMT), then set this domain groups cpu_power such that each group
	 * can handle only one task, when there are other idle groups in the
	 * same sched domain.
	 */
	if (!child || (!(sd->flags & SD_POWERSAVINGS_BALANCE) &&
		       (child->flags &
			(SD_SHARE_CPUPOWER | SD_SHARE_PKG_RESOURCES)))) {
		sg_inc_cpu_power(sd->groups, SCHED_LOAD_SCALE);
		return;
	}

	/*
	 * add cpu_power of each child group to this groups cpu_power
	 */
	group = child->groups;
	do {
		sg_inc_cpu_power(sd->groups, group->__cpu_power);
		group = group->next;
	} while (group != child->groups);
}


/*
 * Build sched domains for a given set of cpus and attach the sched domains
 * to the individual cpus
 */
static int build_sched_domains(const cpumask_t *cpu_map)
{
	int i;
	cpumask_t nodemask_online;

	/*
	 * Set up domains for cpus specified by the cpu_map.
	 */
	for_each_cpu(i, cpu_map) {
		struct sched_domain *sd = NULL, *p;
		cpumask_t nodemask = __cpu_online_mask;

		cpumask_and(&nodemask, &nodemask, cpu_map);

		p = sd;
		sd = &phys_domains[i];
		*sd = SD_CPU_INIT;
		sd->span = nodemask;
		sd->parent = p;
		if (p)
			p->child = sd;
		cpu_to_phys_group(i, cpu_map, &sd->groups);
	}

	nodemask_online = __cpu_online_mask;

	cpumask_and(&nodemask_online, &nodemask_online, cpu_map);

	init_sched_build_groups(nodemask_online, cpu_map, &cpu_to_phys_group);

	for_each_cpu(i, cpu_map) {
		struct sched_domain *sd = &phys_domains[i];

		init_sched_groups_power(i, sd);
	}

	/* Attach the domains */
	for_each_cpu(i, cpu_map) {
		struct sched_domain *sd;

		sd = &phys_domains[i];
		cpu_attach_domain(sd, i);
	}

	return 0;
}

static cpumask_t *doms_cur;	/* current sched domains */
static int ndoms_cur;		/* number of sched domains in 'doms_cur' */

/*
 * Special case: If a kmalloc of a doms_cur partition (array of
 * cpumask_t) fails, then fallback to a single sched domain,
 * as determined by the single cpumask_t fallback_doms.
 */
static cpumask_t fallback_doms;

/*
 * Set up scheduler domains and groups. Callers must hold the hotplug lock.
 * For now this just excludes isolated cpus, but could be used to
 * exclude other special cases in the future.
 */
static int arch_init_sched_domains(const cpumask_t *cpu_map)
{
	int err;

	ndoms_cur = 1;

	doms_cur = &fallback_doms;
	cpumask_andnot(doms_cur, cpu_map, &cpu_isolated_map);
	err = build_sched_domains(doms_cur);

	return err;
}

static void arch_destroy_sched_domains(const cpumask_t *cpu_map)
{
}

/*
 * Detach sched domains from a group of cpus specified in cpu_map
 * These cpus will now be attached to the NULL domain
 */
static void detach_destroy_domains(const cpumask_t *cpu_map)
{
	int i;

	for_each_cpu(i, cpu_map)
		cpu_attach_domain(NULL, i);

	arch_destroy_sched_domains(cpu_map);
}

/*
 * Partition sched domains as specified by the 'ndoms_new'
 * cpumasks in the array doms_new[] of cpumasks. This compares
 * doms_new[] to the current sched domain partitioning, doms_cur[].
 * It destroys each deleted domain and builds each new domain.
 *
 * 'doms_new' is an array of cpumask_t's of length 'ndoms_new'.
 * The masks don't intersect (don't overlap.) We should setup one
 * sched domain for each mask. CPUs not in any of the cpumasks will
 * not be load balanced. If the same cpumask appears both in the
 * current 'doms_cur' domains and in the new 'doms_new', we can leave
 * it as it is.
 *
 * The passed in 'doms_new' should be kmalloc'd. This routine takes
 * ownership of it and will kfree it when done with it. If the caller
 * failed the kmalloc call, then it can pass in doms_new == NULL,
 * and partition_sched_domains() will fallback to the single partition
 * 'fallback_doms'.
 *
 * Call with hotplug lock held
 */
void partition_sched_domains(int ndoms_new, cpumask_t *doms_new)
{
	int i, j;

	if (doms_new == NULL) {
		ndoms_new = 1;
		doms_new = &fallback_doms;
		cpumask_andnot(&doms_new[0], cpu_online_mask, &cpu_isolated_map);
	}

	/* Destroy deleted domains */
	for (i = 0; i < ndoms_cur; i++) {
		for (j = 0; j < ndoms_new; j++) {
			if (cpumask_equal(&doms_cur[i], &doms_new[j]))
				goto match1;
		}
		/* no match - a current sched domain not in new doms_new[] */
		detach_destroy_domains(doms_cur + i);
match1:
		;
	}

	/* Build new domains */
	for (i = 0; i < ndoms_new; i++) {
		for (j = 0; j < ndoms_cur; j++) {
			if (cpumask_equal(&doms_new[i], &doms_cur[j]))
				goto match2;
		}
		/* no match - add a new doms_new */
		build_sched_domains(doms_new + i);
match2:
		;
	}

	doms_cur = doms_new;
	ndoms_cur = ndoms_new;
}

void __init sched_init_smp(void)
{
	cpumask_t non_isolated_cpus;

	mutex_lock(&sched_hotcpu_mutex);
	arch_init_sched_domains(cpu_online_mask);
	cpumask_andnot(&non_isolated_cpus, cpu_possible_mask, &cpu_isolated_map);
	if (cpumask_empty(&non_isolated_cpus))
		cpumask_set_cpu(smp_processor_id(), &non_isolated_cpus);
	mutex_unlock(&sched_hotcpu_mutex);

	sched_init_granularity();
}

int in_sched_functions(unsigned long addr)
{
	return in_lock_functions(addr) ||
		(addr >= (unsigned long)__sched_text_start
		&& addr < (unsigned long)__sched_text_end);
}

static void init_cfs_rq(struct cfs_rq *cfs_rq, struct rq *rq)
{
	cfs_rq->tasks_timeline = RB_ROOT;

	cfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

void __init sched_init(void)
{
	int highest_cpu = 0;
	int i, j;

	for_each_possible_cpu(i) {
		struct rt_prio_array *array;
		struct rq *rq;

		rq = cpu_rq(i);
		spin_lock_init(&rq->lock);
		lockdep_set_class(&rq->lock, &rq->rq_lock_key);
		rq->nr_running = 0;
		rq->clock = 1;
		init_cfs_rq(&rq->cfs, rq);

		for (j = 0; j < CPU_LOAD_IDX_MAX; j++)
			rq->cpu_load[j] = 0;

		rq->sd = NULL;
		rq->active_balance = 0;
		rq->next_balance = jiffies;
		rq->push_cpu = 0;
		rq->cpu = i;
		rq->migration_thread = NULL;
		INIT_LIST_HEAD(&rq->migration_queue);

		atomic_set(&rq->nr_iowait, 0);

		array = &rq->rt.active;
		for (j = 0; j < MAX_RT_PRIO; j++) {
			INIT_LIST_HEAD(array->queue + j);
			__clear_bit(j, array->bitmap);
		}
		highest_cpu = i;
		/* delimiter for bitsearch: */
		__set_bit(MAX_RT_PRIO, array->bitmap);
	}

	set_load_weight(idle_threads);

	open_softirq(SCHED_SOFTIRQ, run_rebalance_domains, NULL);

	/*
	 * The boot idle thread does lazy MMU switching as well:
	 */
	atomic_inc(&init_mm.mm_count);

	/*
	 * Make us the idle thread. Technically, schedule() should not be
	 * called from this thread, however somewhere below it might be,
	 * but because we are the idle thread, we just pick up running again
	 * when this runqueue becomes "idle".
	 */
	init_idle(current, smp_processor_id());
}
