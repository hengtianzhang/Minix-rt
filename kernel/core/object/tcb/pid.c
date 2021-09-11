#include <base/common.h>
#include <base/init.h>

#include <sel4m/sched.h>
#include <sel4m/sched/idle.h>
#include <sel4m/object/pid.h>

static struct rb_root pid_root = RB_ROOT;

static struct pid_struct *pid_find(pid_t pid)
{
	struct rb_node *node = pid_root.rb_node;

	while (node) {
		struct pid_struct *data = container_of(node, struct pid_struct, node);
	
		if (pid < data->pid)
			node = node->rb_left;
		else if (pid > data->pid)
			node = node->rb_right;
		else
			return data;
	}

	return NULL;
}

/**
 * find_process_by_pid - find a process with a matching PID value.
 * @pid: the pid in question.
 */
struct task_struct *find_process_by_pid(pid_t pid)
{
	struct pid_struct *pids;

	pids = pid_find(pid);
	if (!pids)
		return NULL;

	return container_of(pids, struct task_struct, pid);
}

static bool pid_insert(struct pid_struct *data)
{
	struct rb_node **new = &(pid_root.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct pid_struct *this = container_of(*new, struct pid_struct, node);

		parent = *new;
		if (data->pid < this->pid)
			new = &((*new)->rb_left);
		else if (data->pid > this->pid)
			new = &((*new)->rb_right);
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, &pid_root);

	return true;
}

bool insert_process_by_pid(struct task_struct *tsk)
{
	if (!tsk)
		return false;

	return pid_insert(&tsk->pid);
}

bool remove_pid_by_process(struct task_struct *tsk)
{
	struct pid_struct *pids;

	if (!tsk)
		return false;

	pids = pid_find(tsk->pid.pid);
	if (!pids)
		return false;

	rb_erase(&pids->node, &pid_root);

	return true;
}

pid_t pid_first(void)
{
	struct rb_node *node;
	struct pid_struct *pids;

	node = rb_first(&pid_root);
	if (!node)
		return INT_MAX;

	pids = rb_entry(node, struct pid_struct, node);

	return pids->pid;
}

pid_t pid_next(pid_t pid)
{
	struct rb_node *node;
	struct pid_struct *pids;

	pids = pid_find(pid);
	if (!pids)
		return INT_MAX;

	node = rb_next(&pids->node);
	if (!node)
		return INT_MAX;

	pids = rb_entry(node, struct pid_struct, node);

	return pids->pid;
}

void __init process_pid_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		idle_threads[cpu].pid.pid = -cpu;
		if (!insert_process_by_pid(&idle_threads[cpu]))
			BUG();
	}
}
