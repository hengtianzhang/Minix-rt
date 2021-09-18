#ifndef __UAPI_ASM_SYSCALLS_H_
#define __UAPI_ASM_SYSCALLS_H_

#ifndef __SYSCALL
#define __SYSCALL(x, y)
#endif

#define __NR_no_syscall (-1)
__SYSCALL(__NR_no_syscall, sys_ni_syscall)
#define __NR_debug_printf (-2)
__SYSCALL(__NR_debug_printf, sys_debug_printf)

#define __NR_syscalls (-3)

#endif /* !__UAPI_ASM_SYSCALLS_H_ */
