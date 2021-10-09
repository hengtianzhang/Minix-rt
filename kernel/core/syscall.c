#include <base/errno.h>

#include <minix_rt/syscalls.h>

SYSCALL_DEFINE0(ni_syscall)
{
    return -ENOSYS;
}
