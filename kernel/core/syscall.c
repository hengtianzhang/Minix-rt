#include <base/errno.h>

#include <sel4m/syscalls.h>

SYSCALL_DEFINE0(ni_syscall)
{
    return -ENOSYS;
}
