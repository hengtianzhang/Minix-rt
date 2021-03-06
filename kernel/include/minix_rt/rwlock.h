#ifndef __MINIX_RT_RWLOCK_H_
#define __MINIX_RT_RWLOCK_H_

#ifndef __MINIX_RT_SPINLOCK_H_
#error "please don't include this file directly"
#endif

/*
 * rwlock related methods
 *
 * split out from spinlock.h
 *
 * portions Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 */
#define rwlock_init(lock)					\
	do { *(lock) = __RW_LOCK_UNLOCKED(lock); } while (0)

#ifndef arch_read_lock_flags
#define arch_read_lock_flags(lock, flags)	arch_read_lock(lock)
#endif

#ifndef arch_write_lock_flags
#define arch_write_lock_flags(lock, flags)	arch_write_lock(lock)
#endif

#define do_raw_read_lock(rwlock)	do {__acquire(lock); arch_read_lock(&(rwlock)->raw_lock); } while (0)
#define do_raw_read_lock_flags(lock, flags) \
		do {__acquire(lock); arch_read_lock_flags(&(lock)->raw_lock, *(flags)); } while (0)
#define do_raw_read_trylock(rwlock)	arch_read_trylock(&(rwlock)->raw_lock)
#define do_raw_read_unlock(rwlock)	do {arch_read_unlock(&(rwlock)->raw_lock); __release(lock); } while (0)
#define do_raw_write_lock(rwlock)	do {__acquire(lock); arch_write_lock(&(rwlock)->raw_lock); } while (0)
#define do_raw_write_lock_flags(lock, flags) \
		do {__acquire(lock); arch_write_lock_flags(&(lock)->raw_lock, *(flags)); } while (0)
#define do_raw_write_trylock(rwlock)	arch_write_trylock(&(rwlock)->raw_lock)
#define do_raw_write_unlock(rwlock)	do {arch_write_unlock(&(rwlock)->raw_lock); __release(lock); } while (0)

/*
 * Define the various rw_lock methods.  Note we define these
 * regardless of whether CONFIG_SMP or CONFIG_PREEMPT are set. The various
 * methods are defined as nops in the case they are not required.
 */
#define read_trylock(lock)	__cond_lock(lock, _raw_read_trylock(lock))
#define write_trylock(lock)	__cond_lock(lock, _raw_write_trylock(lock))

#define write_lock(lock)	_raw_write_lock(lock)
#define read_lock(lock)		_raw_read_lock(lock)

#define read_lock_irqsave(lock, flags)			\
	do {						\
		typecheck(u64, flags);	\
		flags = _raw_read_lock_irqsave(lock);	\
	} while (0)
#define write_lock_irqsave(lock, flags)			\
	do {						\
		typecheck(u64, flags);	\
		flags = _raw_write_lock_irqsave(lock);	\
	} while (0)

#define read_lock_irq(lock)		_raw_read_lock_irq(lock)
#define write_lock_irq(lock)		_raw_write_lock_irq(lock)
#define read_unlock(lock)		_raw_read_unlock(lock)
#define write_unlock(lock)		_raw_write_unlock(lock)
#define read_unlock_irq(lock)		_raw_read_unlock_irq(lock)
#define write_unlock_irq(lock)		_raw_write_unlock_irq(lock)

#define read_unlock_irqrestore(lock, flags)			\
	do {							\
		typecheck(u64, flags);		\
		_raw_read_unlock_irqrestore(lock, flags);	\
	} while (0)

#define write_unlock_irqrestore(lock, flags)		\
	do {						\
		typecheck(u64, flags);	\
		_raw_write_unlock_irqrestore(lock, flags);	\
	} while (0)

#define write_trylock_irqsave(lock, flags) \
({ \
	local_irq_save(flags); \
	write_trylock(lock) ? \
	1 : ({ local_irq_restore(flags); 0; }); \
})

#endif /* !__MINIX_RT_RWLOCK_H_ */
