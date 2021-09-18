#ifndef __SEL4M_OBJECT_IPC_H_
#define __SEL4M_OBJECT_IPC_H_

struct task_struct;

extern int
ipc_create_task_ipcptr(struct task_struct *tsk, unsigned long ipcptr);

#endif /* !__SEL4M_OBJECT_IPC_H_ */
