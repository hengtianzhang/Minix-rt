#ifndef __SEL4M_RWLOCK_TYPES_H_
#define __SEL4M_RWLOCK_TYPES_H_

/*
 * include/linux/rwlock_types.h - generic rwlock type definitions
 *				  and initializers
 *
 * portions Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 */
typedef struct {
	arch_rwlock_t raw_lock;
} rwlock_t;

#define __RW_LOCK_UNLOCKED(lockname) \
	(rwlock_t)	{	.raw_lock = __ARCH_RW_LOCK_UNLOCKED,	\
	}

#define DEFINE_RWLOCK(x)	rwlock_t x = __RW_LOCK_UNLOCKED(x)

#endif /* !__SEL4M_RWLOCK_TYPES_H_ */
