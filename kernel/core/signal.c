#include <sel4m/signal.h>
#include <sel4m/sched.h>
#include <sel4m/thread.h>
#include <sel4m/object/tcb.h>

#include <asm/current.h>

static void do_default_signal_handle(int signal)
{
	struct task_struct *tsk, *temp_tsk;

	list_for_each_entry_safe(tsk, temp_tsk, &current->children_exit, children_list) {
		if (tsk->exit_code) {
			;
		}
		printf("tsk %s\n", tsk->comm);
		printf("tsk %d\n", tsk->pid.pid);
		printf("tsk  flags %d\n", tsk->flags);
		printf("tsk  exitsignal %d\n", tsk->exit_signal);
		printf("tsk  exitcode %d\n", tsk->exit_code);
		printf("tsk  exitstate %d\n", tsk->exit_state);
		printf("tsk  state %d\n", tsk->state);
	}
}

int do_send_signal(int signal, pid_t pid, int flags)
{
	if (signal == SIGCHLD) {
		BUG_ON(!current->parent);
		notifier_table_set_notifier(SIGCHLD, &current->parent->notifier.notifier_table);
		tcb_do_exit(current, flags);
		BUG();
	} else {
		printf("TODO: signal %d pid %d flags %d\n", signal, pid, flags);
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
