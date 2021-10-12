#ifndef __MINIX_RT_OBJECT_TCB_H_
#define __MINIX_RT_OBJECT_TCB_H_

#include <minix_rt/pid.h>

#include <uapi/minix_rt/magic.h>

struct task_struct;

extern struct task_struct *task_create_tsk(unsigned int flags);
extern void task_destroy_tsk(struct task_struct *tsk);

extern void task_set_stack_end_magic(struct task_struct *tsk);

#define tcb_stack_end_corrupted(tsk)	\
		(*(end_of_stack(tsk)) != STACK_END_MAGIC)

extern void tcb_do_exit(struct task_struct *tsk, int flags);

#endif /* !__MINIX_RT_OBJECT_TCB_H_ */
