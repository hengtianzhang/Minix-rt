#include <base/linkage.h>
#include <base/common.h>

#include <asm/esr.h>

asmlinkage void el0_svc_handler(struct pt_regs *regs)
{
    BUG_ON(1);
}
