/*
 * idle-task scheduling class.
 *
 * (NOTE: these are not related to SCHED_IDLE tasks which are
 *  handled in sched_fair.c)
 */

#include <sel4m/sched.h>
#include <sel4m/sched/idle.h>
#include <sel4m/thread.h>

/* Linker adds these: start and end of __cpuidle functions */
extern char __cpuidle_text_start[], __cpuidle_text_end[];

void __weak arch_cpu_idle_enter(void) { }
void __weak arch_cpu_idle_exit(void) { }
void __weak arch_cpu_idle(void)
{
	local_irq_enable();
	cpu_relax();
}

/**
 * default_idle_call - Default CPU idle routine.
 *
 * To use when the cpuidle framework cannot be used.
 */
void __cpuidle default_idle_call(void)
{
	if (current_clr_polling_and_test()) {
		local_irq_enable();
	} else {
		arch_cpu_idle();
	}
}

/**
 * cpuidle_idle_call - the main idle function
 *
 * NOTE: no locks or semaphores should be used here
 *
 * On archs that support TIF_POLLING_NRFLAG, is called with polling
 * set, and it returns with polling set.  If it ever stops polling, it
 * must clear the polling bit.
 */
static void cpuidle_idle_call(void)
{
	local_irq_disable();

	/*
	 * Check if the idle task must be rescheduled. If it is the
	 * case, exit the function after re-enabling the local irq.
	 */
	if (need_resched()) {
		local_irq_enable();
		return;
	}

	default_idle_call();

	__current_set_polling();

	/*
	 * It is up to the idle functions to reenable local interrupts
	 */
	if (WARN_ON_ONCE(irqs_disabled()))
		local_irq_enable();
}

void cpu_idle(void)
{
	__current_set_polling();

	while (1) {
		rmb();

		arch_cpu_idle_enter();
		select_nohz_load_balancer(1);
		while (!need_resched())
			cpuidle_idle_call();

		select_nohz_load_balancer(0);
		arch_cpu_idle_exit();

		preempt_enable_no_resched();
		schedule();
		preempt_disable();
	}
}

/*
 * Idle tasks are unconditionally rescheduled:
 */
static void check_preempt_curr_idle(struct rq *rq, struct task_struct *p)
{
	resched_task(rq->idle);
}

static struct task_struct *pick_next_task_idle(struct rq *rq)
{
	return rq->idle;
}

/*
 * It is not legal to sleep in the idle task - print a warning
 * message if some code attempts to do it:
 */
static void
dequeue_task_idle(struct rq *rq, struct task_struct *p, int sleep)
{
	spin_unlock_irq(&rq->lock);
	printf("bad: scheduling from the idle thread!\n");
	spin_lock_irq(&rq->lock);
}

static void put_prev_task_idle(struct rq *rq, struct task_struct *prev)
{
}

static unsigned long
load_balance_idle(struct rq *this_rq, int this_cpu, struct rq *busiest,
		  unsigned long max_load_move,
		  struct sched_domain *sd, enum cpu_idle_type idle,
		  int *all_pinned, int *this_best_prio)
{
	return 0;
}

static int
move_one_task_idle(struct rq *this_rq, int this_cpu, struct rq *busiest,
		   struct sched_domain *sd, enum cpu_idle_type idle)
{
	return 0;
}

static void task_tick_idle(struct rq *rq, struct task_struct *curr)
{
}

static void set_curr_task_idle(struct rq *rq)
{
}

/*
 * Simple, special scheduling class for the per-CPU idle tasks:
 */
const struct sched_class idle_sched_class = {
	.next               = NULL,
	/* no enqueue/yield_task for idle tasks */

	/* dequeue is not valid, we print a debug message there: */
	.dequeue_task		= dequeue_task_idle,

	.check_preempt_curr	= check_preempt_curr_idle,

	.pick_next_task		= pick_next_task_idle,
	.put_prev_task		= put_prev_task_idle,

	.load_balance		= load_balance_idle,
	.move_one_task		= move_one_task_idle,

	.set_curr_task          = set_curr_task_idle,
	.task_tick		= task_tick_idle,
	/* no .task_new for idle tasks */
};
