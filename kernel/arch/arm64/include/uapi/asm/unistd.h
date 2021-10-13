#ifndef __UAPI_ASM_SYSCALLS_H_
#define __UAPI_ASM_SYSCALLS_H_

#ifndef __SYSCALL
#define __SYSCALL(x, y)
#endif

#define __NR_rt_sigreturn (139)
__SYSCALL(__NR_rt_sigreturn, sys_rt_sigreturn)

#define __NR_notifier (295)
__SYSCALL(__NR_notifier, sys_notifier)

#define __NR_debug_printf (296)
__SYSCALL(__NR_debug_printf, sys_debug_printf)

#define __NR_syscalls (297)

#endif /* !__UAPI_ASM_SYSCALLS_H_ */
