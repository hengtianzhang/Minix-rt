#include <base/linkage.h>
#include <base/common.h>

#include <sel4m/sched.h>
#include <sel4m/object/tcb.h>

#include <asm/current.h>
#include <asm/esr.h>

asmlinkage void el0_svc_handler(struct pt_regs *regs)
{
    printf("Come in Syscall, nr %lld\n", regs->regs[0]);
    printf("sssssssipc %lld\n", *((u64 *)current->cap_ipcptr + 3));
    printf("aa %d\n", tcb_stack_end_corrupted(current));
    WARN_ON(1);
}
