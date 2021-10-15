#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <base/common.h>
#include <libminix_rt/brk.h>
#include <libminix_rt/ipc.h>

int main(void)
{
	void *addr1;

	printf("This is pm\n");

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

	addr1 = malloc(0x123);
	printf("aaaaa addr1 0x%p\n", addr1);
	free(addr1);

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
