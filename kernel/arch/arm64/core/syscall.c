#include <base/linkage.h>
#include <base/common.h>

#include <asm/esr.h>

asmlinkage void el0_svc_handler(struct pt_regs *regs)
{
    printf("Come in Syscall, nr %lld\n", regs->regs[0]);
    WARN_ON(1);
}
