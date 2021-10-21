#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libminix_rt/ipc.h>

int main(void)
{
	int ret = 0;
	message_t m;

	while (1) {
		memset(&m, 0, sizeof (message_t));

		ret = ipc_receive(ENDPOINT_PM, &m);
		if (ret) {
			panic("PM receive message fail!\n");
		}

		ret = ipc_reply(ENDPOINT_PM, &m);
		if (ret) {
			panic("PM reply message fail!\n");
		}
	};
	BUG();

	return 0;
}
