#include <stdio.h>
#include <stdlib.h>

#include <libminix_rt/pid.h>

#include <base/common.h>

static struct rb_root pid_root = RB_ROOT;

struct pid_struct *pid_find(pid_t pid)
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

bool pid_insert(struct pid_struct *data)
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

bool pid_remove(pid_t pid)
{
	struct pid_struct *pids;

	pids = pid_find(pid);
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
