#ifndef __LIBMINIX_RT_EXEC_H_
#define __LIBMINIX_RT_EXEC_H_

#include <minix_rt/binfmt.h>

int execve(struct minix_rt_binprm *bprm);

#endif /* !__LIBMINIX_RT_EXEC_H_ */
