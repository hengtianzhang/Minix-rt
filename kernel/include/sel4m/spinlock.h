/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SPINLOCK_H_
#define __SEL4M_SPINLOCK_H_

#include <base/common.h>

#include <sel4m/preempt.h>
#include <sel4m/irqflags.h>

/*
 * include/linux/spinlock.h - generic spinlock/rwlock declarations
 *
 * here's the role of the various spinlock/rwlock related include files:
 *
 * on SMP builds:
 *
 *  asm/spinlock_types.h: contains the arch_spinlock_t/arch_rwlock_t and the
 *                        initializers
 *
 *  linux/spinlock_types.h:
 *                        defines the generic type and initializers
 *
 *  asm/spinlock.h:       contains the arch_spin_*()/etc. lowlevel
 *                        implementations, mostly inline assembly code
 *
 *   (also included on UP-debug builds:)
 *
 *  linux/spinlock_api_smp.h:
 *                        contains the prototypes for the _spin_*() APIs.
 *
 *  linux/spinlock.h:     builds the final spin_*() APIs.
 *
 * on UP builds:
 *
 *  linux/spinlock_type_up.h:
 *                        contains the generic, simplified UP spinlock type.
 *                        (which is an empty structure on non-debug builds)
 *
 *  linux/spinlock_types.h:
 *                        defines the generic type and initializers
 *
 *  linux/spinlock_up.h:
 *                        contains the arch_spin_*()/etc. version of UP
 *                        builds. (which are NOPs on non-debug, non-preempt
 *                        builds)
 *
 *   (included on UP-non-debug builds:)
 *
 *  linux/spinlock_api_up.h:
 *                        builds the _spin_*() APIs.
 *
 *  linux/spinlock.h:     builds the final spin_*() APIs.
 */
#include <base/typecheck.h>

#define __lockfunc __attribute__((section(".spinlock.text")))

/*
 * Pull the arch_spinlock_t and arch_rwlock_t definitions:
 */
#include <sel4m/spinlock_types.h>

/*
 * Pull the arch_spin*() functions/declarations (UP-nondebug doesn't need them):
 */
#include <asm/base/barrier.h>

#include <asm-generic/qrwlock.h>
#include <asm-generic/qspinlock.h>

#define raw_spin_lock_init(lock)				\
	do { *(lock) = __RAW_SPIN_LOCK_UNLOCKED(lock); } while (0)

#define raw_spin_is_locked(lock)	arch_spin_is_locked(&(lock)->raw_lock)

#ifdef arch_spin_is_contended
#define raw_spin_is_contended(lock)	arch_spin_is_contended(&(lock)->raw_lock)
#else
#define raw_spin_is_contended(lock)	(((void)(lock), 0))
#endif /*arch_spin_is_contended*/

/*
 * Despite its name it doesn't necessarily has to be a full barrier.
 * It should only guarantee that a STORE before the critical section
 * can not be reordered with a LOAD inside this section.
 * spin_lock() is the one-way barrier, this LOAD can not escape out
 * of the region. So the default implementation simply ensures that
 * a STORE can not move into the critical section, smp_wmb() should
 * serialize it with another STORE done by spin_lock().
 */
#ifndef smp_mb__before_spinlock
#define smp_mb__before_spinlock()	smp_wmb()
#endif

/*
 * Place this after a lock-acquisition primitive to guarantee that
 * an UNLOCK+LOCK pair act as a full barrier.  This guarantee applies
 * if the UNLOCK and LOCK are executed by the same CPU or if the
 * UNLOCK and LOCK operate on the same lock variable.
 */
#ifndef smp_mb__after_unlock_lock
#define smp_mb__after_unlock_lock()	do { } while (0)
#endif

/*
 * smp_mb__after_spinlock() provides the equivalent of a full memory barrier
 * between program-order earlier lock acquisitions and program-order later
 * memory accesses.
 *
 * This guarantees that the following two properties hold:
 *
 *   1) Given the snippet:
 *
 *	  { X = 0;  Y = 0; }
 *
 *	  CPU0				CPU1
 *
 *	  WRITE_ONCE(X, 1);		WRITE_ONCE(Y, 1);
 *	  spin_lock(S);			smp_mb();
 *	  smp_mb__after_spinlock();	r1 = READ_ONCE(X);
 *	  r0 = READ_ONCE(Y);
 *	  spin_unlock(S);
 *
 *      it is forbidden that CPU0 does not observe CPU1's store to Y (r0 = 0)
 *      and CPU1 does not observe CPU0's store to X (r1 = 0); see the comments
 *      preceding the call to smp_mb__after_spinlock() in __schedule() and in
 *      try_to_wake_up().
 *
 *   2) Given the snippet:
 *
 *  { X = 0;  Y = 0; }
 *
 *  CPU0		CPU1				CPU2
 *
 *  spin_lock(S);	spin_lock(S);			r1 = READ_ONCE(Y);
 *  WRITE_ONCE(X, 1);	smp_mb__after_spinlock();	smp_rmb();
 *  spin_unlock(S);	r0 = READ_ONCE(X);		r2 = READ_ONCE(X);
 *			WRITE_ONCE(Y, 1);
 *			spin_unlock(S);
 *
 *      it is forbidden that CPU0's critical section executes before CPU1's
 *      critical section (r0 = 1), CPU2 observes CPU1's store to Y (r1 = 1)
 *      and CPU2 does not observe CPU0's store to X (r2 = 0); see the comments
 *      preceding the calls to smp_rmb() in try_to_wake_up() for similar
 *      snippets but "projected" onto two CPUs.
 *
 * Property (2) upgrades the lock to an RCsc lock.
 *
 * Since most load-store architectures implement ACQUIRE with an smp_mb() after
 * the LL/SC loop, they need no further barriers. Similarly all our TSO
 * architectures imply an smp_mb() for each atomic instruction and equally don't
 * need more.
 *
 * Architectures that can implement ACQUIRE better need to take care.
 */
#ifndef smp_mb__after_spinlock
#define smp_mb__after_spinlock()	do { } while (0)
#endif

static inline void do_raw_spin_lock(raw_spinlock_t *lock) __acquires(lock)
{
	__acquire(lock);
	arch_spin_lock(&lock->raw_lock);
}

#ifndef arch_spin_lock_flags
#define arch_spin_lock_flags(lock, flags)	arch_spin_lock(lock)
#endif

static inline void
do_raw_spin_lock_flags(raw_spinlock_t *lock, u64 *flags) __acquires(lock)
{
	__acquire(lock);
	arch_spin_lock_flags(&lock->raw_lock, *flags);
}

static inline int do_raw_spin_trylock(raw_spinlock_t *lock)
{
	return arch_spin_trylock(&(lock)->raw_lock);
}

static inline void do_raw_spin_unlock(raw_spinlock_t *lock) __releases(lock)
{
	arch_spin_unlock(&lock->raw_lock);
	__release(lock);
}

/*
 * Define the various spin_lock methods.  Note we define these
 * regardless of whether CONFIG_SMP or CONFIG_PREEMPT are set. The
 * various methods are defined as nops in the case they are not
 * required.
 */
#define raw_spin_trylock(lock)	__cond_lock(lock, _raw_spin_trylock(lock))

#define raw_spin_lock(lock)	_raw_spin_lock(lock)

#define raw_spin_lock_nested(lock, subclass)		_raw_spin_lock(lock)
#define raw_spin_lock_nest_lock(lock, nest_lock)	_raw_spin_lock(lock)

#define raw_spin_lock_irqsave(lock, flags)			\
	do {						\
		typecheck(u64, flags);	\
		flags = _raw_spin_lock_irqsave(lock);	\
	} while (0)

#define raw_spin_lock_irq(lock)		_raw_spin_lock_irq(lock)
#define raw_spin_unlock(lock)		_raw_spin_unlock(lock)
#define raw_spin_unlock_irq(lock)	_raw_spin_unlock_irq(lock)

#define raw_spin_unlock_irqrestore(lock, flags)		\
	do {							\
		typecheck(u64, flags);		\
		_raw_spin_unlock_irqrestore(lock, flags);	\
	} while (0)

#define raw_spin_trylock_irq(lock) \
({ \
	local_irq_disable(); \
	raw_spin_trylock(lock) ? \
	1 : ({ local_irq_enable(); 0;  }); \
})

#define raw_spin_trylock_irqsave(lock, flags) \
({ \
	local_irq_save(flags); \
	raw_spin_trylock(lock) ? \
	1 : ({ local_irq_restore(flags); 0; }); \
})

/* Include rwlock functions */
#include <sel4m/rwlock.h>

/*
 * Pull the _spin_*()/_read_*()/_write_*() functions/declarations:
 */
#include <sel4m/spinlock_api_smp.h>

/*
 * Map the spin_lock functions to the raw variants for PREEMPT_RT=n
 */
static __always_inline raw_spinlock_t *spinlock_check(spinlock_t *lock)
{
	return &lock->rlock;
}

#define spin_lock_init(_lock)				\
do {							\
	spinlock_check(_lock);				\
	raw_spin_lock_init(&(_lock)->rlock);		\
} while (0)

static __always_inline void spin_lock(spinlock_t *lock)
{
	raw_spin_lock(&lock->rlock);
}

static __always_inline int spin_trylock(spinlock_t *lock)
{
	return raw_spin_trylock(&lock->rlock);
}

#define spin_lock_nested(lock, subclass)			\
do {								\
	raw_spin_lock_nested(spinlock_check(lock), subclass);	\
} while (0)

#define spin_lock_nest_lock(lock, nest_lock)				\
do {									\
	raw_spin_lock_nest_lock(spinlock_check(lock), nest_lock);	\
} while (0)

static __always_inline void spin_lock_irq(spinlock_t *lock)
{
	raw_spin_lock_irq(&lock->rlock);
}

#define spin_lock_irqsave(lock, flags)				\
do {								\
	raw_spin_lock_irqsave(spinlock_check(lock), flags);	\
} while (0)

static __always_inline void spin_unlock(spinlock_t *lock)
{
	raw_spin_unlock(&lock->rlock);
}

static __always_inline void spin_unlock_irq(spinlock_t *lock)
{
	raw_spin_unlock_irq(&lock->rlock);
}

static __always_inline void spin_unlock_irqrestore(spinlock_t *lock, u64 flags)
{
	raw_spin_unlock_irqrestore(&lock->rlock, flags);
}

static __always_inline int spin_trylock_irq(spinlock_t *lock)
{
	return raw_spin_trylock_irq(&lock->rlock);
}

#define spin_trylock_irqsave(lock, flags)			\
({								\
	raw_spin_trylock_irqsave(spinlock_check(lock), flags); \
})

/**
 * spin_is_locked() - Check whether a spinlock is locked.
 * @lock: Pointer to the spinlock.
 *
 * This function is NOT required to provide any memory ordering
 * guarantees; it could be used for debugging purposes or, when
 * additional synchronization is needed, accompanied with other
 * constructs (memory barriers) enforcing the synchronization.
 *
 * Returns: 1 if @lock is locked, 0 otherwise.
 *
 * Note that the function only tells you that the spinlock is
 * seen to be locked, not that it is locked on your CPU.
 *
 * Further, on CONFIG_SMP=n builds with CONFIG_DEBUG_SPINLOCK=n,
 * the return value is always 0 (see include/linux/spinlock_up.h).
 * Therefore you should not rely heavily on the return value.
 */
static __always_inline int spin_is_locked(spinlock_t *lock)
{
	return raw_spin_is_locked(&lock->rlock);
}

static __always_inline int spin_is_contended(spinlock_t *lock)
{
	return raw_spin_is_contended(&lock->rlock);
}

#define assert_spin_locked(lock)	assert_raw_spin_locked(&(lock)->rlock)

/*
 * Pull the atomic_t declaration:
 * (asm-mips/atomic.h needs above definitions)
 */
#include <base/atomic.h>
/**
 * atomic_dec_and_lock - lock on reaching reference count zero
 * @atomic: the atomic counter
 * @lock: the spinlock in question
 *
 * Decrements @atomic by 1.  If the result is 0, returns true and locks
 * @lock.  Returns false for all other cases.
 */
extern int _atomic_dec_and_lock(atomic_t *atomic, spinlock_t *lock);
#define atomic_dec_and_lock(atomic, lock) \
		__cond_lock(lock, _atomic_dec_and_lock(atomic, lock))

extern int _atomic_dec_and_lock_irqsave(atomic_t *atomic, spinlock_t *lock,
					u64 *flags);
#define atomic_dec_and_lock_irqsave(atomic, lock, flags) \
		__cond_lock(lock, _atomic_dec_and_lock_irqsave(atomic, lock, &(flags)))

#endif /* !__SEL4M_SPINLOCK_H_ */
