#ifndef __SEL4M_OBJECT_TCB_H_
#define __SEL4M_OBJECT_TCB_H_

#include <sel4m/object/pid.h>

struct task_struct;

extern struct task_struct *tcb_create_task(void);
extern void tcb_destroy_task(struct task_struct *tsk);

#endif /* !__SEL4M_OBJECT_TCB_H_ */
