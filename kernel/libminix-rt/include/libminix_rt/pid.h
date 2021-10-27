#ifndef __LIBMINIX_RT_PID_H_
#define __LIBMINIX_RT_PID_H_

#include <base/types.h>
#include <base/rbtree.h>

struct pid_struct {
	pid_t   pid;
	struct rb_node  node;
};

struct pid_struct *pid_find(pid_t pid);
bool pid_insert(struct pid_struct *data);
bool pid_remove(pid_t pid);

pid_t pid_first(void);
pid_t pid_next(pid_t pid);

#define pid_for_each_pid(pid)   \
	for (pid = pid_first(); pid != INT_MAX; pid = pid_next(pid))

#endif /* !__LIBMINIX_RT_PID_H_ */
