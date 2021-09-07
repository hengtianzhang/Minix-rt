#include <base/cache.h>

#include <sel4m/sched.h>

struct rq runqueues[CONFIG_NR_CPUS] __cacheline_aligned_in_smp;

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

void __update_rq_clock(struct rq *rq)
{

}

void update_rq_clock(struct rq *rq)
{

}
