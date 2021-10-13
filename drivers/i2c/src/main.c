#include <stdio.h>
#include <string.h>

#include <libminix_rt/ipc.h>

int main(void)
{
	printf("This is i2c!\n");
	while (1) {
		int ret = 0;
		message_t m;

		memset(&m, 0, sizeof (message_t));

		printf("i2c send 0x95\n");
		m.m_u64.data[1] = 0x95;
		ret = ipc_send(ENDPOINT_SYSTEM, &m);
		if (ret) {
			printf("I2C send message fail!\n");
		}
	};
	return 0;
}
