#include <base/common.h>
#include <base/init.h>

#include <sel4m/slab.h>
#include <sel4m/sched.h>
#include <sel4m/sched/idle.h>
#include <sel4m/object/pid.h>

struct pid_cache {
	pid_t	pid;
	struct list_head list;
};

static struct rb_root pid_root = RB_ROOT;
static LIST_HEAD(pid_root_cache);

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
 * pid_find_process_by_pid - find a process with a matching PID value.
 * @pid: the pid in question.
 */
struct task_struct *pid_find_process_by_pid(pid_t pid)
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

bool pid_insert_process_by_pid(struct task_struct *tsk)
{
	if (!tsk)
		return false;

	return pid_insert(&tsk->pid);
}

int pid_alloc_pid(struct task_struct *tsk)
{
	pid_t pid;
	struct rb_node *node;
	struct pid_struct *pids;

	if (!tsk)
		return -EINVAL;

	if (!list_empty(&pid_root_cache)) {
		struct pid_cache *pidc;
		pidc = list_first_entry(&pid_root_cache, struct pid_cache, list);
		pid = pidc->pid;

		list_del(&pidc->list);
		kfree(pidc);

		tsk->pid.pid = pid;
		BUG_ON(!pid_insert_process_by_pid(tsk));

		return 0;
	}

	node = rb_last(&pid_root);
	BUG_ON(!node);

	pids = rb_entry(node, struct pid_struct, node);

	tsk->pid.pid = pids->pid + 1;
	BUG_ON(!pid_insert_process_by_pid(tsk));

	return 0;
}

bool pid_remove_pid_by_process(struct task_struct *tsk)
{
	struct pid_struct *pids;

	if (!tsk)
		return false;

	pids = pid_find(tsk->pid.pid);
	if (!pids)
		return false;

	if (&pids->node != rb_last(&pid_root)) {
		struct pid_cache *pidc;

		pidc = kmalloc(sizeof (struct pid_cache), GFP_KERNEL);
		if (!pidc)
			return false;

		pidc->pid = tsk->pid.pid;
		list_add(&pidc->list, &pid_root_cache);
	}

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
		if (!pid_insert_process_by_pid(&idle_threads[cpu]))
			BUG();
	}
}
