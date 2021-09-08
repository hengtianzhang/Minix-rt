/*
 * idle-task scheduling class.
 *
 * (NOTE: these are not related to SCHED_IDLE tasks which are
 *  handled in sched_fair.c)
 */

#include <sel4m/sched.h>
#include <sel4m/sched/idle.h>

struct task_struct idle_threads[CONFIG_NR_CPUS];

__visible __attribute__((aligned(THREAD_STACK_ALIGN)))
u8 idle_stack[CONFIG_NR_CPUS][THREAD_SIZE];

void __init idle_prepare_init(struct task_struct *idle, int cpu)
{
	idle->stack = &idle_stack[cpu];
	idle->cpu = cpu;
	idle->mm = &init_mm;
	idle->pid = -cpu;
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
