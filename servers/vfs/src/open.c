#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libminix_rt/mmap.h>
#include <libminix_rt/ipc.h>

#include "fs.h"
#include "open.h"

void do_openat(endpoint_t ep, message_t *m)
{
	printf("TODO openat! dfd %d %d\n", m->m_vfs_openat.dfd, AT_FDCWD);
	while(1);
}
