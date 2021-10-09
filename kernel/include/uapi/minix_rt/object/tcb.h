#ifndef __UAPI_MINIX_RT_OBJECT_TCB_H_
#define __UAPI_MINIX_RT_OBJECT_TCB_H_

enum tcb_table {
    tcb_create_thread_fn,
    tcb_create_tcb_object,
    tcb_clone_task_fn,
    tcb_get_pid,
};

#endif /* !__UAPI_MINIX_RT_OBJECT_TCB_H_ */
