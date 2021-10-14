#include <stdio.h>
#include <string.h>

#include <libminix_rt/brk.h>
#include <libminix_rt/ipc.h>

int main(void)
{
	int ret __maybe_unused;
	void *addr;
	printf("This is pm\n");

	addr = sbrk(0);
	if (!addr)
		return -1;

	printf("current heap %p\n", addr);

	while (1) {
		int ret = 0;
		message_t m;

		memset(&m, 0, sizeof (message_t));

		m.m_u64.data[1] = 0x1ff;
		ret = ipc_send(ENDPOINT_SYSTEM, &m);
		if (ret) {
			printf("pm send message fail!\n");
		}
	};
	return 0;
}
