#ifndef __MINIX_RT_SCHED_FAIR_H_
#define __MINIX_RT_SCHED_FAIR_H_

extern unsigned int sysctl_sched_latency;
extern unsigned int sysctl_sched_min_granularity;
extern unsigned int sysctl_sched_batch_wakeup_granularity;
extern unsigned int sysctl_sched_wakeup_granularity;
extern const unsigned int sysctl_sched_migration_cost;

static inline struct cfs_rq *task_cfs_rq(struct task_struct *p)
{
	return &task_rq(p)->cfs;
}

static inline struct cfs_rq *cpu_cfs_rq(struct cfs_rq *cfs_rq, int this_cpu)
{
	return &cpu_rq(this_cpu)->cfs;
}

#endif /* !__MINIX_RT_SCHED_FAIR_H_ */
