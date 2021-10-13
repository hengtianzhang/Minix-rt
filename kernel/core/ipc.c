#include <base/common.h>
#include <base/string.h>
#include <base/errno.h>

#include <minix_rt/sched.h>
#include <minix_rt/ipc.h>

static struct endpoint_info ipc_ep_info[] = {
	[ENDPOINT_SYSTEM] = {
		.endpoint = ENDPOINT_SYSTEM,
		.name = INIT_SERVICE_COMM,
		.tsk = NULL,
	},
	[ENDPOINT_I2C] = {
		.endpoint = ENDPOINT_I2C,
		.name = "i2c",
		.tsk = NULL,
	},
	[ENDPOINT_PM] = {
		.endpoint = ENDPOINT_PM,
		.name = "pm",
		.tsk = NULL,
	},
	[ENDPOINT_VFS] = {
		.endpoint = ENDPOINT_VFS,
		.name = "vfs",
		.tsk = NULL,
	},
};

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
				return 0;
			}
		}
	}
	return -ENODEV;
}

int __ipc_send(endpoint_t dest, message_t *m_ptr)
{
	return 0;
}
