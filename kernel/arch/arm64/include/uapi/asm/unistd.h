#ifndef __UAPI_ASM_SYSCALLS_H_
#define __UAPI_ASM_SYSCALLS_H_

#ifndef __SYSCALL
#define __SYSCALL(x, y)
#endif

#define __NR_rt_sigreturn (139)
__SYSCALL(__NR_rt_sigreturn, sys_rt_sigreturn)
#define __NR_getuid 174
__SYSCALL(__NR_getuid, sys_getuid)
#define __NR_geteuid 175
__SYSCALL(__NR_geteuid, sys_geteuid)
#define __NR_getgid 176
__SYSCALL(__NR_getgid, sys_getgid)
#define __NR_getegid 177
__SYSCALL(__NR_getegid, sys_getegid)
#define __NR_brk 214
__SYSCALL(__NR_brk, sys_brk)

#define __NR_notifier (295)
__SYSCALL(__NR_notifier, sys_notifier)

#define __NR_debug_printf (296)
__SYSCALL(__NR_debug_printf, sys_debug_printf)

#define __NR_ipc_send (297)
__SYSCALL(__NR_ipc_send, sys_ipc_send)

#define __NR_ipc_receive (298)
__SYSCALL(__NR_ipc_receive, sys_ipc_receive)

#define __NR_ipc_reply (299)
__SYSCALL(__NR_ipc_reply, sys_ipc_reply)

#define __NR_ipc_notify (300)
__SYSCALL(__NR_ipc_notify, sys_ipc_notify)

#define __NR_syscalls (301)

#endif /* !__UAPI_ASM_SYSCALLS_H_ */
