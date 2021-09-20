#ifndef __SEL4M_STAT_H_
#define __SEL4M_STAT_H_

#include <base/compiler.h>
#include <base/linkage.h>

void do_exit(long error_code) __noreturn;

void dump_stack_set_arch_desc(const char *fmt, ...);

extern asmlinkage void dump_stack(void) __cold;

struct pt_regs;
void show_regs(struct pt_regs * regs);

void show_regs_print_info(void);

#define system_running() (likely(system_state == SYSTEM_RUNNING))

#endif /* !__SEL4M_STAT_H_ */
