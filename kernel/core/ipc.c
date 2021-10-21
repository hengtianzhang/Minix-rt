#include <base/common.h>
#include <base/string.h>
#include <base/errno.h>

#include <minix_rt/syscalls.h>
#include <minix_rt/smp.h>
#include <minix_rt/slab.h>
#include <minix_rt/spinlock.h>
#include <minix_rt/sched.h>
#include <minix_rt/ipc.h>
#include <minix_rt/jiffies.h>

static struct endpoint_info ipc_ep_info[] = {
	[ENDPOINT_SYSTEM] = {
		.endpoint = ENDPOINT_SYSTEM,
		.name = INIT_SERVICE_COMM,
	},
	[ENDPOINT_I2C] = {
		.endpoint = ENDPOINT_I2C,
		.name = "i2c",
	},
	[ENDPOINT_PM] = {
		.endpoint = ENDPOINT_PM,
		.name = "pm",
	},
	[ENDPOINT_VFS] = {
		.endpoint = ENDPOINT_VFS,
		.name = "vfs",
	},
};

void ipc_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ipc_ep_info); i++) {
		struct endpoint_info *ep_info = &ipc_ep_info[i];
		ep_info->state = EP_STATE_NONE;
		ep_info->tsk = NULL;
		ep_info->ipc_req_count = 0;
		INIT_LIST_HEAD(&ep_info->ep_list);
		spin_lock_init(&ep_info->ep_list_lock);
		spin_lock_init(&ep_info->ep_lock);
		init_waitqueue_head(&ep_info->wait);
	}
}

int ipc_register_endpoint_by_tsk(struct task_struct *tsk)
{
	int i, len_endpoint, len_out;

	if (!tsk)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(ipc_ep_info); i++) {
		len_endpoint = strlen(ipc_ep_info[i].name);
		len_out = strlen(tsk->comm);
		if (len_out == len_endpoint) {
			if (strncmp(ipc_ep_info[i].name, tsk->comm, len_out) == 0) {
				if (ipc_ep_info[i].tsk)
					return -EBUSY;
				tsk->ep = ipc_ep_info[i].endpoint;
				ipc_ep_info[i].tsk = tsk;
				ipc_ep_info[i].state = EP_STATE_RUNNING;
				wake_up(&ipc_ep_info[i].wait);
				return 0;
			}
		}
	}
	return -ENODEV;
}

int __ipc_send(endpoint_t dest, message_t *m_ptr)
{
	struct ipc_mess_node *ep_node;
	struct endpoint_info *ep_info;
	u64 flags;

	if (dest > ENDPOINT_END || !m_ptr)
		return -EINVAL;

	ep_info = &ipc_ep_info[dest];
	if (unlikely(!ep_info->tsk))
		if (!wait_event_timeout(ep_info->wait,
				ep_info->state == EP_STATE_RUNNING,
				jiffies_to_nsecs(60 * HZ)))
			return -ENODEV;

	ep_node = kmalloc(sizeof (struct ipc_mess_node), GFP_KERNEL | GFP_ZERO);
	if (!ep_node)
		return -ENOMEM;

	m_ptr->m_source = current->pid.pid;
	memcpy(&ep_node->m, m_ptr, sizeof (message_t));
	ep_node->is_finish = 0;
	init_waitqueue_head(&ep_node->wait);

	spin_lock_irqsave(&ep_info->ep_list_lock, flags);
	list_add_tail(&ep_node->queue_list, &ep_info->ep_list);
	if (ep_info->state == EP_STATE_WAITTING) {
		ep_info->state = EP_STATE_RUNNING;
		spin_unlock_irqrestore(&ep_info->ep_list_lock, flags);
		wake_up(&ep_info->wait);
	} else
		spin_unlock_irqrestore(&ep_info->ep_list_lock, flags);

	if (!(m_ptr->m_type & IPC_M_TYPE_NOTIFIER)) {
		wait_event(ep_node->wait, ep_node->is_finish);
		memcpy(m_ptr, &ep_node->m, sizeof (message_t));
		kfree(ep_node);
	}

	return 0;
}

int __ipc_receive(endpoint_t src, message_t *m_ptr)
{
	struct ipc_mess_node *ep_node;
	struct endpoint_info *ep_info;
	u64 flags;

	if (src > ENDPOINT_END || !m_ptr)
		return -EINVAL;

	ep_info = &ipc_ep_info[src];
	if (unlikely(!ep_info->tsk))
		return -ENODEV;

	if (ep_info->tsk != current)
		return -EACCES;

	spin_lock(&ep_info->ep_lock);
	if (ep_info->ipc_req_count != 0) {
		spin_unlock(&ep_info->ep_lock);
		return -EBUSY;
	}
	spin_unlock(&ep_info->ep_lock);

	spin_lock_irqsave(&ep_info->ep_list_lock, flags);
	if (list_empty(&ep_info->ep_list)) {
		ep_info->state = EP_STATE_WAITTING;
		spin_unlock_irqrestore(&ep_info->ep_list_lock, flags);
		wait_event(ep_info->wait, ep_info->state == EP_STATE_RUNNING);
	} else {
		spin_unlock_irqrestore(&ep_info->ep_list_lock, flags);
	}

	spin_lock_irqsave(&ep_info->ep_list_lock, flags);
	ep_node = list_first_entry(&ep_info->ep_list, struct ipc_mess_node, queue_list);
	spin_unlock_irqrestore(&ep_info->ep_list_lock, flags);

	memcpy(m_ptr, &ep_node->m, sizeof (message_t));

	spin_lock(&ep_info->ep_lock);
	ep_info->ipc_req_count += 1;
	spin_unlock(&ep_info->ep_lock);

	return 0;
}

int __ipc_reply(endpoint_t src, message_t *m_ptr)
{
	struct ipc_mess_node *ep_node;
	struct endpoint_info *ep_info;
	u64 flags;

	if (src > ENDPOINT_END || !m_ptr)
		return -EINVAL;

	ep_info = &ipc_ep_info[src];
	if (unlikely(!ep_info->tsk))
		return -ENODEV;

	if (ep_info->tsk != current)
		return -EACCES;

	spin_lock(&ep_info->ep_lock);
	if (ep_info->ipc_req_count == 0) {
		spin_unlock(&ep_info->ep_lock);
		return -EINVAL;
	}
	spin_unlock(&ep_info->ep_lock);

	spin_lock_irqsave(&ep_info->ep_list_lock, flags);
	ep_node = list_first_entry(&ep_info->ep_list, struct ipc_mess_node, queue_list);
	list_del(&ep_node->queue_list);
	spin_unlock_irqrestore(&ep_info->ep_list_lock, flags);

	if (ep_node->m.m_type & IPC_M_TYPE_NOTIFIER) {
		kfree(ep_node);
	} else {
		ep_node->is_finish = 1;
		memcpy(&ep_node->m, m_ptr, sizeof (message_t));
		wake_up(&ep_node->wait);
	}

	spin_lock(&ep_info->ep_lock);
	ep_info->ipc_req_count -= 1;
	spin_unlock(&ep_info->ep_lock);

	return 0;
}

int __ipc_notify(endpoint_t dest, message_t *m_ptr)
{
	if (!m_ptr)
		return -EINVAL;

	m_ptr->m_type |= IPC_M_TYPE_NOTIFIER;
	return __ipc_send(dest, m_ptr);
}

SYSCALL_DEFINE2(ipc_send, endpoint_t, dest, message_t __user *, m_ptr)
{
	int ret;
	message_t m;

	if(copy_from_user(&m, m_ptr, sizeof (message_t)))
		return -EFAULT;

	ret = __ipc_send(dest, &m);
	if (ret)
		return ret;

	return copy_to_user(m_ptr, &m, sizeof (message_t));
}

SYSCALL_DEFINE2(ipc_receive, endpoint_t, src, message_t __user *, m_ptr)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	ret = __ipc_receive(src, &m);
	if (ret)
		return ret;

	return copy_to_user(m_ptr, &m, sizeof (message_t));
}

SYSCALL_DEFINE2(ipc_reply, endpoint_t, src, message_t __user *, m_ptr)
{
	message_t m;

	if(copy_from_user(&m, m_ptr, sizeof (message_t)))
		return -EFAULT;

	return __ipc_reply(src, &m);
}

SYSCALL_DEFINE2(ipc_notify, endpoint_t, dest, message_t __user *, m_ptr)
{
	message_t m;

	if(copy_from_user(&m, m_ptr, sizeof (message_t)))
		return -EFAULT;

	return __ipc_notify(dest, &m);
}
