#ifndef __UAPI_SEL4M_OBJECT_NOTIFIER_H_
#define __UAPI_SEL4M_OBJECT_NOTIFIER_H_

#include <base/bitmap.h>
#include <base/const.h>

#ifndef __KERNEL__
#include <asm/signal.h>
#else
#include <uapi/asm/signal.h>
#endif /* !__KERNEL__ */

#define NOTIFIER_IRQ		(SIGRTMAX + 1)
#define NOTIFIER_TASK_EXIT	(NOTIFIER_IRQ + 1)
#define NOTIFIER_NR_MAX		(NOTIFIER_TASK_EXIT + 1)

#define notifier_nr_max		((unsigned int)NOTIFIER_NR_MAX)

typedef struct notifier_table { DECLARE_BITMAP(notifier_bits, NOTIFIER_NR_MAX); } notifier_table_t;

#define notifier_table_bits(notifierp) ((notifierp)->notifier_bits)

/**
 * notifier_table_first - get the first notifier in a notifier_table
 * @srcp: the notifier_table pointer
 *
 * Returns >= notifier_nr_max if no notifiers set.
 */
static inline unsigned int notifier_table_first(const struct notifier_table *srcp)
{
	return find_first_bit(notifier_table_bits(srcp), notifier_nr_max);
}

/**
 * notifier_table_next - get the next notifier in a notifier_table
 * @n: the notifier prior to the place to search (ie. return will be > @n)
 * @srcp: the notifier_table pointer
 *
 * Returns >= notifier_nr_max if no further notifiers set.
 */
static inline unsigned int notifier_table_next(int n, const struct notifier_table *srcp)
{
	return find_next_bit(notifier_table_bits(srcp), notifier_nr_max, n + 1);
}

/**
 * notifier_table_next_and - get the next notifier in *src1p & *src2p
 * @n: the notifier prior to the place to search (ie. return will be > @n)
 * @src1p: the first notifier_table pointer
 * @src2p: the second notifier_table pointer
 *
 * Returns >= notifier_nr_max if no further notifiers set in both.
 */
static inline int notifier_table_next_and(int n, const struct notifier_table *src1p,
						const struct notifier_table *src2p)
{
	return find_next_and_bit(notifier_table_bits(src1p), notifier_table_bits(src2p),
		notifier_nr_max, n + 1);
}

/**
 * notifier_table_last - get the last notifier in a notifier_table
 * @srcp:	- the notifier_table pointer
 *
 * Returns	>= notifier_nr_max if no notifiers set.
 */
static inline unsigned int notifier_table_last(const struct notifier_table *srcp)
{
	return find_last_bit(notifier_table_bits(srcp), notifier_nr_max);
}

/**
 * notifier_table_next_zero - get the next unset notifier in a notifier_table
 * @n: the notifier prior to the place to search (ie. return will be > @n)
 * @srcp: the notifier_table pointer
 *
 * Returns >= notifier_nr_max if no further notifiers unset.
 */
static inline unsigned int notifier_table_next_zero(int n, const struct notifier_table *srcp)
{
	return find_next_zero_bit(notifier_table_bits(srcp), notifier_nr_max, n + 1);
}

/**
 * notifier_table_set_notifier - set a notifier in a notifier_table
 * @notifier: notifier number (< notifier_nr_max)
 * @dstp: the notifier_table pointer
 */
static inline void notifier_table_set_notifier(unsigned int notifier, struct notifier_table *dstp)
{
	set_bit(notifier, notifier_table_bits(dstp));
}

/**
 * notifier_table_clear_notifier - clear a notifier in a notifier_table
 * @notifier: notifier number (< notifier_nr_max)
 * @dstp: the notifier_table pointer
 */
static inline void notifier_table_clear_notifier(unsigned int notifier, struct notifier_table *dstp)
{
	clear_bit(notifier, notifier_table_bits(dstp));
}

/**
 * notifier_table_test_notifier - test for a notifier in a notifier_table
 * @notifier: notifier number (< notifier_nr_max)
 * @notifier_table: the notifier_table pointer
 *
 * Returns 1 if @notifier is set in @notifier_table, else returns 0
 */
static inline int notifier_table_test_notifier(unsigned int notifier, struct notifier_table *notifier_table)
{
	return test_bit(notifier, notifier_table_bits((notifier_table)));
}

/**
 * notifier_table_test_set_notifier - atomically test and set a notifier in a notifier_table
 * @notifier: notifier number (< notifier_nr_max)
 * @notifier_table: the notifier_table pointer
 *
 * Returns 1 if @notifier is set in old bitmap of @notifier_table, else returns 0
 *
 * test_and_set_bit wrapper for notifier_table.
 */
static inline int notifier_table_test_set_notifier(unsigned int notifier, struct notifier_table *notifier_table)
{
	return test_and_set_bit(notifier, notifier_table_bits((notifier_table)));
}

/**
 * notifier_table_test_clear_notifier - atomically test and clear a notifier in a notifier_table
 * @notifier: notifier number (< notifier_nr_max)
 * @notifier_table: the notifier_table pointer
 *
 * Returns 1 if @notifier is set in old bitmap of @notifier_table, else returns 0
 *
 * test_and_clear_bit wrapper for notifier_table.
 */
static inline int notifier_table_test_clear_notifier(int notifier, struct notifier_table *notifier_table)
{
	return test_and_clear_bit(notifier, notifier_table_bits((notifier_table)));
}

/**
 * notifier_table_setall - set all notifiers (< notifier_nr_max) in a notifier_table
 * @dstp: the notifier_table pointer
 */
static inline void notifier_table_setall(struct notifier_table *dstp)
{
	bitmap_fill(notifier_table_bits(dstp), notifier_nr_max);
}

/**
 * notifier_table_clearall - clear all notifiers (< notifier_nr_max) in a notifier_table
 * @dstp: the notifier_table pointer
 */
static inline void notifier_table_clearall(struct notifier_table *dstp)
{
	bitmap_zero(notifier_table_bits(dstp), notifier_nr_max);
}

/**
 * notifier_table_and - *dstp = *src1p & *src2p
 * @dstp: the notifier_table result
 * @src1p: the first input
 * @src2p: the second input
 *
 * If *@dstp is empty, returns 0, else returns 1
 */
static inline int notifier_table_and(struct notifier_table *dstp,
			       const struct notifier_table *src1p,
			       const struct notifier_table *src2p)
{
	return bitmap_and(notifier_table_bits(dstp), notifier_table_bits(src1p),
				       notifier_table_bits(src2p), notifier_nr_max);
}

/**
 * notifier_table_or - *dstp = *src1p | *src2p
 * @dstp: the notifier_table result
 * @src1p: the first input
 * @src2p: the second input
 */
static inline void notifier_table_or(struct notifier_table *dstp, const struct notifier_table *src1p,
			      const struct notifier_table *src2p)
{
	bitmap_or(notifier_table_bits(dstp), notifier_table_bits(src1p),
				      notifier_table_bits(src2p), notifier_nr_max);
}

/**
 * notifier_table_xor - *dstp = *src1p ^ *src2p
 * @dstp: the notifier_table result
 * @src1p: the first input
 * @src2p: the second input
 */
static inline void notifier_table_xor(struct notifier_table *dstp,
			       const struct notifier_table *src1p,
			       const struct notifier_table *src2p)
{
	bitmap_xor(notifier_table_bits(dstp), notifier_table_bits(src1p),
				       notifier_table_bits(src2p), notifier_nr_max);
}

/**
 * notifier_table_andnot - *dstp = *src1p & ~*src2p
 * @dstp: the notifier_table result
 * @src1p: the first input
 * @src2p: the second input
 *
 * If *@dstp is empty, returns 0, else returns 1
 */
static inline int notifier_table_andnot(struct notifier_table *dstp,
				  const struct notifier_table *src1p,
				  const struct notifier_table *src2p)
{
	return bitmap_andnot(notifier_table_bits(dstp), notifier_table_bits(src1p),
					  notifier_table_bits(src2p), notifier_nr_max);
}

/**
 * notifier_table_complement - *dstp = ~*srcp
 * @dstp: the notifier_table result
 * @srcp: the input to invert
 */
static inline void notifier_table_complement(struct notifier_table *dstp,
				      const struct notifier_table *srcp)
{
	bitmap_complement(notifier_table_bits(dstp), notifier_table_bits(srcp),
					      notifier_nr_max);
}

/**
 * notifier_table_equal - *src1p == *src2p
 * @src1p: the first input
 * @src2p: the second input
 */
static inline bool notifier_table_equal(const struct notifier_table *src1p,
				const struct notifier_table *src2p)
{
	return bitmap_equal(notifier_table_bits(src1p), notifier_table_bits(src2p),
						 notifier_nr_max);
}

/**
 * notifier_table_intersects - (*src1p & *src2p) != 0
 * @src1p: the first input
 * @src2p: the second input
 */
static inline bool notifier_table_intersects(const struct notifier_table *src1p,
				     const struct notifier_table *src2p)
{
	return bitmap_intersects(notifier_table_bits(src1p), notifier_table_bits(src2p),
						      notifier_nr_max);
}

/**
 * notifier_table_subset - (*src1p & ~*src2p) == 0
 * @src1p: the first input
 * @src2p: the second input
 *
 * Returns 1 if *@src1p is a subset of *@src2p, else returns 0
 */
static inline int notifier_table_subset(const struct notifier_table *src1p,
				 const struct notifier_table *src2p)
{
	return bitmap_subset(notifier_table_bits(src1p), notifier_table_bits(src2p),
						  notifier_nr_max);
}

/**
 * notifier_table_empty - *srcp == 0
 * @srcp: the notifier_table to that all notifiers < notifier_nr_max are clear.
 */
static inline bool notifier_table_empty(const struct notifier_table *srcp)
{
	return bitmap_empty(notifier_table_bits(srcp), notifier_nr_max);
}

/**
 * notifier_table_full - *srcp == 0xFFFFFFFF...
 * @srcp: the notifier_table to that all notifiers < notifier_nr_max are set.
 */
static inline bool notifier_table_full(const struct notifier_table *srcp)
{
	return bitmap_full(notifier_table_bits(srcp), notifier_nr_max);
}

/**
 * notifier_table_weight - Count of bits in *srcp
 * @srcp: the notifier_table to count bits (< notifier_nr_max) in.
 */
static inline unsigned int notifier_table_weight(const struct notifier_table *srcp)
{
	return bitmap_weight(notifier_table_bits(srcp), notifier_nr_max);
}

/**
 * notifier_table_shift_right - *dstp = *srcp >> n
 * @dstp: the notifier_table result
 * @srcp: the input to shift
 * @n: the number of bits to shift by
 */
static inline void notifier_table_shift_right(struct notifier_table *dstp,
				       const struct notifier_table *srcp, int n)
{
	bitmap_shift_right(notifier_table_bits(dstp), notifier_table_bits(srcp), n,
					       notifier_nr_max);
}

/**
 * notifier_table_shift_left - *dstp = *srcp << n
 * @dstp: the notifier_table result
 * @srcp: the input to shift
 * @n: the number of bits to shift by
 */
static inline void notifier_table_shift_left(struct notifier_table *dstp,
				      const struct notifier_table *srcp, int n)
{
	bitmap_shift_left(notifier_table_bits(dstp), notifier_table_bits(srcp), n,
					      notifier_nr_max);
}

/**
 * notifier_table_copy - *dstp = *srcp
 * @dstp: the result
 * @srcp: the input notifier_table
 */
static inline void notifier_table_copy(struct notifier_table *dstp,
				const struct notifier_table *srcp)
{
	bitmap_copy(notifier_table_bits(dstp), notifier_table_bits(srcp), notifier_nr_max);
}

/**
 * notifier_table_any - pick a "random" notifier from *srcp
 * @srcp: the input notifier_table
 *
 * Returns >= notifier_nr_max if no notifiers set.
 */
#define notifier_table_any(srcp) notifier_table_first(srcp)

/**
 * notifier_table_first_and - return the first notifier from *srcp1 & *srcp2
 * @src1p: the first input
 * @src2p: the second input
 *
 * Returns >= notifier_nr_max if no notifiers set in both.  See also notifier_table_next_and().
 */
#define notifier_table_first_and(src1p, src2p) notifier_table_next_and(-1, (src1p), (src2p))

/**
 * notifier_table_any_and - pick a "random" notifier from *mask1 & *mask2
 * @mask1: the first input notifier_table
 * @mask2: the second input notifier_table
 *
 * Returns >= notifier_nr_max if no notifiers set.
 */
#define notifier_table_any_and(mask1, mask2) notifier_table_first_and((mask1), (mask2))

/**
 * notifier_table_size - size to allocate for a 'struct notifier_table' in bytes
 */
static inline unsigned int notifier_table_size(void)
{
	return BITS_TO_LONGS(notifier_nr_max) * sizeof(long);
}

/**
 * for_each_notifier_table - iterate over every notifier in a mask
 * @notifier: the (optionally unsigned) integer iterator
 * @notifier_table: the notifier_table pointer
 *
 * After the loop, notifier is >= notifier_nr_max.
 */
#define for_each_notifier_table(notifier, notifier_table)	\
	for ((notifier) = -1;				\
		(notifier) = notifier_table_next((notifier), (notifier_table)),	\
		(notifier) < notifier_nr_max;)

/**
 * for_each_notifier_table_not - iterate over every notifier in a complemented mask
 * @notifier: the (optionally unsigned) integer iterator
 * @notifier_table: the notifier_table pointer
 *
 * After the loop, notifier is >= notifier_nr_max.
 */
#define for_each_notifier_table_not(notifier, notifier_table)	\
	for ((notifier) = -1;				\
		(notifier) = notifier_table_next_zero((notifier), (notifier_table)),	\
		(notifier) < notifier_nr_max;)

#define NOTIFIER_TABLE_MASK_NONE						\
(notifier_table_t) {{								\
	[0 ... BITS_TO_LONGS(NOTIFIER_NR_MAX)-1] = 0UL			\
}}

#define NOTIFIER_TABLE_MASK_NOTIFIER0						\
(notifier_table_t) {{								\
	[0] =  1UL						\
}}

typedef void __signalfn_t(int);
typedef __signalfn_t __user *__sighandler_t;

struct sigaction {
	__sighandler_t	sa_handler;
};

struct k_sigaction {
	struct sigaction sa;
};

#define NOTIFIER_DFL	((__force __sighandler_t)0) /* default signal handling */
#define NOTIFIER_IGN	((__force __sighandler_t)1)	/* ignore signal */
#define NOTIFIER_ERR	((__force __sighandler_t)-1) /* error return from signal */

#endif /* !__UAPI_SEL4M_OBJECT_NOTIFIER_H_ */
