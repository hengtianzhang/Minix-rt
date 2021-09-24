#ifndef __UAPI_ASM_SYSCALLS_H_
#define __UAPI_ASM_SYSCALLS_H_

#ifndef __SYSCALL
#define __SYSCALL(x, y)
#endif

#define __NR_no_syscall (-1)
__SYSCALL(__NR_no_syscall, sys_ni_syscall)

#define __NR_untype (-2)
__SYSCALL(__NR_untype, sys_untype)

#define __NR_notifier (-3)
__SYSCALL(__NR_notifier, sys_notifier)

#define __NR_rt_sigreturn (-4)
__SYSCALL(__NR_rt_sigreturn, sys_rt_sigreturn)

#define __NR_tcb_thread (-5)
__SYSCALL(__NR_tcb_thread, sys_tcb_thread)

#define __NR_debug_printf (-6)
__SYSCALL(__NR_debug_printf, sys_debug_printf)

#define __NR_syscalls (-7)

#endif /* !__UAPI_ASM_SYSCALLS_H_ */
