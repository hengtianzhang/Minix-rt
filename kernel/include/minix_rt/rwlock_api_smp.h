#ifndef __MINIX_RT_RWLOCK_API_SMP_H_
#define __MINIX_RT_RWLOCK_API_SMP_H_

#ifndef __MINIX_RT_SPINLOCK_API_SMP_H_
# error "please don't include this file directly"
#endif

/*
 * include/linux/rwlock_api_smp.h
 *
 * spinlock API declarations on SMP (and debug)
 * (implemented in kernel/spinlock.c)
 *
 * portions Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 */
void __lockfunc _raw_read_lock(rwlock_t *lock)		__acquires(lock);
void __lockfunc _raw_write_lock(rwlock_t *lock)		__acquires(lock);
void __lockfunc _raw_read_lock_irq(rwlock_t *lock)	__acquires(lock);
void __lockfunc _raw_write_lock_irq(rwlock_t *lock)	__acquires(lock);
u64 __lockfunc _raw_read_lock_irqsave(rwlock_t *lock)
							__acquires(lock);
u64 __lockfunc _raw_write_lock_irqsave(rwlock_t *lock)
							__acquires(lock);
int __lockfunc _raw_read_trylock(rwlock_t *lock);
int __lockfunc _raw_write_trylock(rwlock_t *lock);
void __lockfunc _raw_read_unlock(rwlock_t *lock)	__releases(lock);
void __lockfunc _raw_write_unlock(rwlock_t *lock)	__releases(lock);
void __lockfunc _raw_read_unlock_irq(rwlock_t *lock)	__releases(lock);
void __lockfunc _raw_write_unlock_irq(rwlock_t *lock)	__releases(lock);
void __lockfunc
_raw_read_unlock_irqrestore(rwlock_t *lock, u64 flags)
							__releases(lock);
void __lockfunc
_raw_write_unlock_irqrestore(rwlock_t *lock, u64 flags)
							__releases(lock);

static inline int __raw_read_trylock(rwlock_t *lock)
{
	preempt_disable();
	if (do_raw_read_trylock(lock))
		return 1;
	preempt_enable();
	return 0;
}

static inline int __raw_write_trylock(rwlock_t *lock)
{
	preempt_disable();
	if (do_raw_write_trylock(lock))
		return 1;
	preempt_enable();
	return 0;
}

static inline void __raw_read_lock(rwlock_t *lock)
{
	preempt_disable();
	do_raw_read_lock(lock);
}

static inline u64 __raw_read_lock_irqsave(rwlock_t *lock)
{
	u64 flags;

	local_irq_save(flags);
	preempt_disable();
	do_raw_read_lock_flags(lock, &flags);

	return flags;
}

static inline void __raw_read_lock_irq(rwlock_t *lock)
{
	local_irq_disable();
	preempt_disable();
	do_raw_read_lock(lock);
}

static inline u64 __raw_write_lock_irqsave(rwlock_t *lock)
{
	u64 flags;

	local_irq_save(flags);
	preempt_disable();
	do_raw_write_lock_flags(lock, &flags);

	return flags;
}

static inline void __raw_write_lock_irq(rwlock_t *lock)
{
	local_irq_disable();
	preempt_disable();
	do_raw_write_lock(lock);
}

static inline void __raw_write_lock(rwlock_t *lock)
{
	preempt_disable();
	do_raw_write_lock(lock);
}

static inline void __raw_write_unlock(rwlock_t *lock)
{
	do_raw_write_unlock(lock);
	preempt_enable();
}

static inline void __raw_read_unlock(rwlock_t *lock)
{
	do_raw_read_unlock(lock);
	preempt_enable();
}

static inline void
__raw_read_unlock_irqrestore(rwlock_t *lock, u64 flags)
{
	do_raw_read_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}

static inline void __raw_read_unlock_irq(rwlock_t *lock)
{
	do_raw_read_unlock(lock);
	local_irq_enable();
	preempt_enable();
}

static inline void __raw_write_unlock_irqrestore(rwlock_t *lock,
					     u64 flags)
{
	do_raw_write_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}

static inline void __raw_write_unlock_irq(rwlock_t *lock)
{
	do_raw_write_unlock(lock);
	local_irq_enable();
	preempt_enable();
}

#endif /* !__MINIX_RT_RWLOCK_API_SMP_H_ */
