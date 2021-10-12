#include <minix_rt/signal.h>
#include <minix_rt/sched.h>
#include <minix_rt/thread.h>
#include <minix_rt/mmap.h>
#include <minix_rt/object/tcb.h>
#include <minix_rt/object/pid.h>
#include <minix_rt/object/ipc.h>

#include <asm/current.h>

static void do_default_signal_handle(int signal)
{
	struct task_struct *tsk, *temp_tsk;

	list_for_each_entry_safe(tsk, temp_tsk, &current->children_exit, children_list) {
		struct task_struct *child;

		if (tsk->exit_code) {
			BUG(); /* TODO  segment fault */
		}

		list_for_each_entry(child, &tsk->children, children_list) {
			child->parent = current;
			list_add(&child->children_list, &current->children);
		}

		list_del(&tsk->children_list);
		mmap_destroy_mm(tsk);
		kfree(tsk->stack);
		pid_remove_pid_by_process(tsk);
		tcb_destroy_task(tsk);
	}
}

int do_send_signal(int signal, pid_t pid, long private)
{
	if (signal == SIGCHLD) {
		BUG_ON(!current->parent);
		notifier_table_set_notifier(SIGCHLD, &current->parent->notifier.notifier_table);
		tcb_do_exit(current, (int)private);
		BUG();
	} else {
		struct task_struct *tsk;
		struct ipc_share_struct *ipc_shr;

		tsk = pid_find_process_by_pid(pid);
		if (!tsk)
			return -EINVAL;
		
		ipc_shr = tsk->kernel_ipcptr;
		ipc_shr->notifier_message_info[signal] = private;
		ipc_shr->notifier[signal] = current->pid.pid;

		notifier_table_set_notifier(signal, &tsk->notifier.notifier_table);
		set_tsk_thread_flag(tsk, TIF_SIGPENDING);
	}

	return 0;
}

bool get_signal(struct ksignal *ksig)
{
	int notifier;

	for_each_notifier_table(notifier, &current->notifier.notifier_table) {
		struct k_sigaction ka = current->notifier.action[notifier];

		notifier_table_clear_notifier(notifier, &current->notifier.notifier_table);
		if (!notifier_table_weight(&current->notifier.notifier_table))
			clear_tsk_thread_flag(current, TIF_SIGPENDING);

		if (ka.sa.sa_handler == NOTIFIER_IGN) {
			continue;
		} else if (ka.sa.sa_handler == NOTIFIER_DFL) {
			do_default_signal_handle(notifier);
			continue;
		} else {
			if (ksig) {
				ksig->ka = ka;
				ksig->sig = notifier;
			}
			clear_tsk_thread_flag(current, TIF_SIGPENDING);
			return true;
		}
	}
	/* TODO clear thread sigpending ? */
	clear_tsk_thread_flag(current, TIF_SIGPENDING);
	return false;
}

void signal_setup_done(int failed, struct ksignal *ksig, int stepping)
{
}
