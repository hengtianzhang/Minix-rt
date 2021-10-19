#include <stdio.h>
#include <string.h>

#include <libminix_rt/ipc.h>

int main(void)
{
	int ret = 0;
	message_t m;

	printf("This is i2c!\n");

	while (1) {
		memset(&m, 0, sizeof (message_t));

		ret = ipc_receive(ENDPOINT_I2C, &m);
		if (ret) {
			panic("I2C receive message fail!\n");
		}

		ret = ipc_reply(ENDPOINT_I2C, &m);
		if (ret) {
			panic("I2C reply message fail!\n");
		}
	};
	BUG();

	return 0;
}
