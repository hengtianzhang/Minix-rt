#include <sel4m/signal.h>
#include <sel4m/sched.h>
#include <sel4m/thread.h>

#include <asm/current.h>

int do_send_signal(int signal, pid_t pid, int flags)
{
	if (signal == SIGCHLD) {
		BUG_ON(!current->parent);
		notifier_table_set_notifier(SIGCHLD, &current->parent->notifier.notifier_table);
		set_tsk_thread_flag(current->parent, TIF_SIGPENDING);
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
			printf("Do default signal handle\n");
			continue;
		} else {
			if (ksig) {
				ksig->ka = ka;
				ksig->sig = notifier;
			}
			return true;
		}
	}

	return false;
}
