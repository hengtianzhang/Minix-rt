#include <sel4m/sched.h>
#include <sel4m/object/tcb.h>

void tcb_do_exit(struct task_struct *tsk, int flags)
{
	tsk->exit_code = flags;
	tsk->exit_signal = SIGCHLD;
	tsk->exit_state = EXIT_ZOMBIE;
	tsk->flags |= PF_EXITING;
	list_del(&tsk->children_list);
	list_add(&tsk->children_list, &tsk->parent->children_exit);

	put_task_struct(tsk);
	preempt_disable();
	tsk->state = TASK_DEAD;
	set_tsk_thread_flag(tsk->parent, TIF_SIGPENDING);
	schedule();
	BUG();
	/* Avoid "noreturn function does return".  */
	for (;;)
		cpu_relax();	/* For when BUG is null */
}
