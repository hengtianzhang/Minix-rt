#include <minix_rt/signal.h>
#include <minix_rt/sched.h>
#include <minix_rt/thread.h>
#include <minix_rt/mmap.h>
#include <minix_rt/pid.h>
#include <minix_rt/syscalls.h>

#include <uapi/minix_rt/notifier.h>

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
		mmap_destroy_mm(tsk->mm);
		kfree(tsk->stack);
		pid_remove_pid_by_process(tsk);
		task_destroy_tsk(tsk);
	}
}

int do_send_signal(int signal, pid_t pid, long private)
{
	if (signal == SIGCHLD) {
		BUG_ON(!current->parent);
		notifier_table_set_notifier(SIGCHLD, &current->parent->notifier.notifier_table);
		task_do_exit(current, (int)private);
		BUG();
	} else {
		struct task_struct *tsk;

		tsk = pid_find_process_by_pid(pid);
		if (!tsk)
			return -EINVAL;
		
		tsk->notifier.private[signal] = private;
		tsk->notifier.pid[signal] = current->pid.pid;

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

static void __set_task_blocked(struct task_struct *tsk, const sigset_t *newset)
{
/* TODO */
//	if (signal_pending(tsk) && !thread_group_empty(tsk)) {
	if (signal_pending(tsk)) {
		sigset_t newblocked;
		/* A set of now blocked but previously unblocked signals. */
		sigandnsets(&newblocked, newset, &current->notifier.blocked);
	//	retarget_shared_pending(tsk, &newblocked);
	}
	tsk->notifier.blocked = *newset;
//	recalc_sigpending();
}

void __set_current_blocked(const sigset_t *newset)
{
	struct task_struct *tsk = current;

	/*
	 * In case the signal mask hasn't changed, there is nothing we need
	 * to do. The current->blocked shouldn't be modified by other task.
	 */
	if (sigequalsets(&tsk->notifier.blocked, newset))
		return;

	spin_lock_irq(&tsk->notifier.siglock);
	__set_task_blocked(tsk, newset);
	spin_unlock_irq(&tsk->notifier.siglock);
}

int sigprocmask(int how, sigset_t *set, sigset_t *oldset)
{
	struct task_struct *tsk = current;
	sigset_t newset;

	/* Lockless, only current can change ->blocked, never from irq */
	if (oldset)
		*oldset = tsk->notifier.blocked;

	switch (how) {
	case SIG_BLOCK:
		sigorsets(&newset, &tsk->notifier.blocked, set);
		break;
	case SIG_UNBLOCK:
		sigandnsets(&newset, &tsk->notifier.blocked, set);
		break;
	case SIG_SETMASK:
		newset = *set;
		break;
	default:
		return -EINVAL;
	}

	__set_current_blocked(&newset);
	return 0;
}

void signal_setup_done(int failed, struct ksignal *ksig, int stepping)
{
}

SYSCALL_DEFINE6(notifier, enum notifier_type, table,
				unsigned int, notifier, unsigned long, fn,
				pid_t, pid, long, private, __sigrestore_t, return_fn)
{
	if (notifier >= NOTIFIER_NR_MAX)
		return -EINVAL;

	switch (table) {
		case notifier_regiser_fn:
			if (BAD_ADDR(fn))
				return -EFAULT;

			current->notifier.action[notifier].sa.sa_handler = (__sighandler_t)fn;
			if (current->notifier.return_fn) {
				if (current->notifier.return_fn != return_fn)
					return -EINVAL;
			}
			current->notifier.return_fn = return_fn;
			return 0;
		case notifier_remove_fn:
			current->notifier.action[notifier].sa.sa_handler = NOTIFIER_DFL;

			return 0;
		case notifier_send_signal:
			if (notifier < SIGRTMAX)
				return do_send_signal(notifier, pid, private);
			else
				printf("send notifier %d nothing TODO\n", notifier);
			return 0;
	}

	return -EINVAL;
}

/**
 *  sys_rt_sigprocmask - change the list of currently blocked signals
 *  @how: whether to add, remove, or set signals
 *  @nset: stores pending signals
 *  @oset: previous value of signal mask if non-null
 *  @sigsetsize: size of sigset_t type
 */
SYSCALL_DEFINE4(rt_sigprocmask, int, how, sigset_t __user *, nset,
		sigset_t __user *, oset, size_t, sigsetsize)
{
	sigset_t old_set, new_set;
	int error;

	if (sigsetsize != sizeof (sigset_t))
		return -EINVAL;

	old_set = current->notifier.blocked;
	if (nset) {
		if (copy_from_user(&new_set, nset, sizeof (sigset_t)))
			return -EFAULT;
		sigdelsetmask(&new_set, sigmask(SIGKILL) | sigmask(SIGSTOP));

		error = sigprocmask(how, &new_set, NULL);
		if (error)
			return error;
	}

	if (oset) {
		if (copy_to_user(oset, &old_set, sizeof(sigset_t)))
			return -EFAULT;
	}

	return 0;
}
