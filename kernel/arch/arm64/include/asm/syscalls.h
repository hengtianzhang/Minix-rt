#ifndef __ASM_MINIX_RT_SYSCALLS_H_
#define __ASM_MINIX_RT_SYSCALLS_H_

#include <asm/ptrace.h>

typedef long (*syscall_fn_t)(struct pt_regs *regs);

extern const syscall_fn_t sys_call_table[];

#define SC_ARM64_REGS_TO_ARGS(x, ...)				\
	__MAP(x,__SC_ARGS					\
		  ,,regs->regs[0],,regs->regs[1],,regs->regs[2]	\
		  ,,regs->regs[3],,regs->regs[4],,regs->regs[5])

#define __SYSCALL_DEFINEx(x, name, ...)						\
	asmlinkage long __arm64_sys##name(const struct pt_regs *regs);		\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__));		\
	static inline long __do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__));	\
	asmlinkage long __arm64_sys##name(const struct pt_regs *regs)		\
	{									\
		return __se_sys##name(SC_ARM64_REGS_TO_ARGS(x,__VA_ARGS__));	\
	}									\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__))		\
	{									\
		long ret = __do_sys##name(__MAP(x,__SC_CAST,__VA_ARGS__));	\
		__MAP(x,__SC_TEST,__VA_ARGS__);					\
		__PROTECT(x, ret,__MAP(x,__SC_ARGS,__VA_ARGS__));		\
		return ret;							\
	}									\
	static inline long __do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))

#ifndef SYSCALL_DEFINE0
#define SYSCALL_DEFINE0(sname)					\
	asmlinkage long __arm64_sys_##sname(void);		\
	asmlinkage long __arm64_sys_##sname(void)
#endif

#endif /* !__ASM_MINIX_RT_SYSCALLS_H_ */
