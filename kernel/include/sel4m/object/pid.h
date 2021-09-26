#ifndef __SEL4M_OBJECT_PID_H_
#define __SEL4M_OBJECT_PID_H_

#include <base/types.h>
#include <base/rbtree.h>

struct pid_struct {
	pid_t   pid;
	struct rb_node  node;
};

struct task_struct *pid_find_process_by_pid(pid_t pid);
bool pid_insert_process_by_pid(struct task_struct *tsk);
bool pid_remove_pid_by_process(struct task_struct *tsk);

void process_pid_init(void);
pid_t pid_first(void);
pid_t pid_next(pid_t pid);

#define pid_for_each_pid(pid)   \
	for (pid = pid_first(); pid != INT_MAX; pid = pid_next(pid))

#endif /* !__SEL4M_OBJECT_PID_H_ */
