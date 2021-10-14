#ifndef __LIBMINIX_RT_BRK_H_
#define __LIBMINIX_RT_BRK_H_

extern void *sbrk(long increment);
extern int brk(void *addr);

#endif /* !__LIBMINIX_RT_BRK_H_ */
