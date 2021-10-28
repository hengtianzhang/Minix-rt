#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libminix_rt/mmap.h>
#include <libminix_rt/ipc.h>

#include "read_write.h"

void do_write(endpoint_t ep, message_t *m)
{
	printf("TODO write!\n");
	while(1);
}

void do_writev(endpoint_t ep, message_t *m)
{
	printf("TODO writev!\n");
	while(1);
}
