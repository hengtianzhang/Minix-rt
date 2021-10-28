#include <minix_rt/system.h>
#include <minix_rt/syscalls.h>
#include <minix_rt/reboot.h>
#include <minix_rt/sched.h>

/*
 * Reboot system call: for obvious reasons only root may call it,
 * and even root needs to set up some magic numbers in the registers
 * so that some mistake won't make this reboot the whole machine.
 * You can also set the meaning of the ctrl-alt-del-key here.
 *
 * reboot doesn't sync: do that yourself before calling this.
 */
SYSCALL_DEFINE4(reboot, int, magic1, int, magic2, unsigned int, cmd,
		void __user *, arg)
{
	/* TODO */
	return 0;
}
