#ifndef __SERVERS_PM_SRC_PM_H_
#define __SERVERS_PM_SRC_PM_H_

#include <libminix_rt/pid.h>

struct task_struct {
	pid_t uid, euid, gid, egid, sid;
	struct pid_struct pid;
};

struct task_struct *pm_find_task(pid_t pid);

#endif /* !__SERVERS_PM_SRC_PM_H_ */
