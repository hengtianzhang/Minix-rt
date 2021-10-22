#include <string.h>

#include <base/random.h>

#include <libminix_rt/syscalls.h>
#include <libminix_rt/ipc.h>

static u64 get_seed(void)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));

	m.m_type = IPC_M_TYPE_SYSTEM_SEED;
	ret = ipc_send(ENDPOINT_SYSTEM, &m);
	if (ret)
		return 0;

	return m.m_u64.data[0];
}

void get_random_bytes(void *buf, int nbytes)
{
	int i;
	char *tmp = buf;
	u64 seed;

	if (!buf)
		return;

	seed = get_seed();
	srand(seed);
	for (i = 0; i < nbytes; i++) {
		tmp[i] = (char)random();
		srand(tmp[i]);
	}
}
