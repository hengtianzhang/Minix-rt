#include <stdio.h>
#include <string.h>

#include <libminix_rt/ipc.h>

int main(void)
{
	printf("This is pm\n");
	while (1) {
		int ret = 0;
		message_t m;

		memset(&m, 0, sizeof (message_t));

		printf("pm send 0x1ff\n");
		m.m_u64.data[1] = 0x1ff;
		ret = ipc_send(ENDPOINT_SYSTEM, &m);
		if (ret) {
			printf("pm send message fail!\n");
		}
	};
	return 0;
}
