#ifndef __SEL4M_OBJECT_CAP_TYPES_H_
#define __SEL4M_OBJECT_CAP_TYPES_H_

#include <base/bitmap.h>
#include <base/const.h>

enum cap_type {
	cap_null_cap,
	cap_cnode_cap,
	cap_frame_cap,
	cap_untyped_cap,
	cap_pgd_table_cap,
	cap_pud_table_cap,
	cap_pmd_table_cap,
	cap_pte_table_cap,
	cap_notification_cap,
	cap_endpoint_cap,
	cap_reply_cap,
	cap_irq_control_cap,
	cap_irq_handler_cap,
	cap_thread_cap,
	__cap_max_cap_end,
};

#define CAP_NR_MAX	__cap_max_cap_end

#define cap_nr_max ((unsigned int)CAP_NR_MAX)

/* Don't assign or return these: may not be this big! */
typedef struct cap_table { DECLARE_BITMAP(cap_bits, CAP_NR_MAX); } cap_table_t;

#define cap_table_bits(capp) ((capp)->cap_bits)

/**
 * cap_table_first - get the first cap in a cap_table
 * @srcp: the cap_table pointer
 *
 * Returns >= cap_nr_max if no caps set.
 */
static inline unsigned int cap_table_first(const struct cap_table *srcp)
{
	return find_first_bit(cap_table_bits(srcp), cap_nr_max);
}

/**
 * cap_table_next - get the next cap in a cap_table
 * @n: the cap prior to the place to search (ie. return will be > @n)
 * @srcp: the cap_table pointer
 *
 * Returns >= cap_nr_max if no further caps set.
 */
static inline unsigned int cap_table_next(int n, const struct cap_table *srcp)
{
	return find_next_bit(cap_table_bits(srcp), cap_nr_max, n + 1);
}

/**
 * cap_table_next_and - get the next cap in *src1p & *src2p
 * @n: the cap prior to the place to search (ie. return will be > @n)
 * @src1p: the first cap_table pointer
 * @src2p: the second cap_table pointer
 *
 * Returns >= cap_nr_max if no further caps set in both.
 */
static inline int cap_table_next_and(int n, const struct cap_table *src1p,
						const struct cap_table *src2p)
{
	return find_next_and_bit(cap_table_bits(src1p), cap_table_bits(src2p),
		cap_nr_max, n + 1);
}

/**
 * cap_table_last - get the last cap in a cap_table
 * @srcp:	- the cap_table pointer
 *
 * Returns	>= cap_nr_max if no caps set.
 */
static inline unsigned int cap_table_last(const struct cap_table *srcp)
{
	return find_last_bit(cap_table_bits(srcp), cap_nr_max);
}

/**
 * cap_table_next_zero - get the next unset cap in a cap_table
 * @n: the cap prior to the place to search (ie. return will be > @n)
 * @srcp: the cap_table pointer
 *
 * Returns >= cap_nr_max if no further caps unset.
 */
static inline unsigned int cap_table_next_zero(int n, const struct cap_table *srcp)
{
	return find_next_zero_bit(cap_table_bits(srcp), cap_nr_max, n + 1);
}

/**
 * cap_table_set_cap - set a cap in a cap_table
 * @cap: cap number (< cap_nr_max)
 * @dstp: the cap_table pointer
 */
static inline void cap_table_set_cap(unsigned int cap, struct cap_table *dstp)
{
	set_bit(cap, cap_table_bits(dstp));
}

/**
 * cap_table_clear_cap - clear a cap in a cap_table
 * @cap: cap number (< cap_nr_max)
 * @dstp: the cap_table pointer
 */
static inline void cap_table_clear_cap(unsigned int cap, struct cap_table *dstp)
{
	clear_bit(cap, cap_table_bits(dstp));
}

/**
 * cap_table_test_cap - test for a cap in a cap_table
 * @cap: cap number (< cap_nr_max)
 * @cap_table: the cap_table pointer
 *
 * Returns 1 if @cap is set in @cap_table, else returns 0
 */
static inline int cap_table_test_cap(unsigned int cap, struct cap_table *cap_table)
{
	return test_bit(cap, cap_table_bits((cap_table)));
}

/**
 * cap_table_test_set_cap - atomically test and set a cap in a cap_table
 * @cap: cap number (< cap_nr_max)
 * @cap_table: the cap_table pointer
 *
 * Returns 1 if @cap is set in old bitmap of @cap_table, else returns 0
 *
 * test_and_set_bit wrapper for cap_table.
 */
static inline int cap_table_test_set_cap(unsigned int cap, struct cap_table *cap_table)
{
	return test_and_set_bit(cap, cap_table_bits((cap_table)));
}

/**
 * cap_table_test_clear_cap - atomically test and clear a cap in a cap_table
 * @cap: cap number (< cap_nr_max)
 * @cap_table: the cap_table pointer
 *
 * Returns 1 if @cap is set in old bitmap of @cap_table, else returns 0
 *
 * test_and_clear_bit wrapper for cap_table.
 */
static inline int cap_table_test_clear_cap(int cap, struct cap_table *cap_table)
{
	return test_and_clear_bit(cap, cap_table_bits((cap_table)));
}

/**
 * cap_table_setall - set all caps (< cap_nr_max) in a cap_table
 * @dstp: the cap_table pointer
 */
static inline void cap_table_setall(struct cap_table *dstp)
{
	bitmap_fill(cap_table_bits(dstp), cap_nr_max);
}

/**
 * cao_table_clearall - clear all caps (< cap_nr_max) in a cap_table
 * @dstp: the cap_table pointer
 */
static inline void cao_table_clearall(struct cap_table *dstp)
{
	bitmap_zero(cap_table_bits(dstp), cap_nr_max);
}

/**
 * cpu_table_and - *dstp = *src1p & *src2p
 * @dstp: the cap_table result
 * @src1p: the first input
 * @src2p: the second input
 *
 * If *@dstp is empty, returns 0, else returns 1
 */
static inline int cpu_table_and(struct cap_table *dstp,
			       const struct cap_table *src1p,
			       const struct cap_table *src2p)
{
	return bitmap_and(cap_table_bits(dstp), cap_table_bits(src1p),
				       cap_table_bits(src2p), cap_nr_max);
}

/**
 * cap_table_or - *dstp = *src1p | *src2p
 * @dstp: the cap_table result
 * @src1p: the first input
 * @src2p: the second input
 */
static inline void cap_table_or(struct cap_table *dstp, const struct cap_table *src1p,
			      const struct cap_table *src2p)
{
	bitmap_or(cap_table_bits(dstp), cap_table_bits(src1p),
				      cap_table_bits(src2p), cap_nr_max);
}

/**
 * cap_table_xor - *dstp = *src1p ^ *src2p
 * @dstp: the cap_table result
 * @src1p: the first input
 * @src2p: the second input
 */
static inline void cap_table_xor(struct cap_table *dstp,
			       const struct cap_table *src1p,
			       const struct cap_table *src2p)
{
	bitmap_xor(cap_table_bits(dstp), cap_table_bits(src1p),
				       cap_table_bits(src2p), cap_nr_max);
}

/**
 * cap_table_andnot - *dstp = *src1p & ~*src2p
 * @dstp: the cap_table result
 * @src1p: the first input
 * @src2p: the second input
 *
 * If *@dstp is empty, returns 0, else returns 1
 */
static inline int cap_table_andnot(struct cap_table *dstp,
				  const struct cap_table *src1p,
				  const struct cap_table *src2p)
{
	return bitmap_andnot(cap_table_bits(dstp), cap_table_bits(src1p),
					  cap_table_bits(src2p), cap_nr_max);
}

/**
 * cap_table_complement - *dstp = ~*srcp
 * @dstp: the cap_table result
 * @srcp: the input to invert
 */
static inline void cap_table_complement(struct cap_table *dstp,
				      const struct cap_table *srcp)
{
	bitmap_complement(cap_table_bits(dstp), cap_table_bits(srcp),
					      cap_nr_max);
}

/**
 * cap_table_equal - *src1p == *src2p
 * @src1p: the first input
 * @src2p: the second input
 */
static inline bool cap_table_equal(const struct cap_table *src1p,
				const struct cap_table *src2p)
{
	return bitmap_equal(cap_table_bits(src1p), cap_table_bits(src2p),
						 cap_nr_max);
}

/**
 * cap_table_intersects - (*src1p & *src2p) != 0
 * @src1p: the first input
 * @src2p: the second input
 */
static inline bool cap_table_intersects(const struct cap_table *src1p,
				     const struct cap_table *src2p)
{
	return bitmap_intersects(cap_table_bits(src1p), cap_table_bits(src2p),
						      cap_nr_max);
}

/**
 * cap_table_subset - (*src1p & ~*src2p) == 0
 * @src1p: the first input
 * @src2p: the second input
 *
 * Returns 1 if *@src1p is a subset of *@src2p, else returns 0
 */
static inline int cap_table_subset(const struct cap_table *src1p,
				 const struct cap_table *src2p)
{
	return bitmap_subset(cap_table_bits(src1p), cap_table_bits(src2p),
						  cap_nr_max);
}

/**
 * cap_table_empty - *srcp == 0
 * @srcp: the cap_table to that all caps < cap_nr_max are clear.
 */
static inline bool cap_table_empty(const struct cap_table *srcp)
{
	return bitmap_empty(cap_table_bits(srcp), cap_nr_max);
}

/**
 * cap_table_full - *srcp == 0xFFFFFFFF...
 * @srcp: the cap_table to that all caps < cap_nr_max are set.
 */
static inline bool cap_table_full(const struct cap_table *srcp)
{
	return bitmap_full(cap_table_bits(srcp), cap_nr_max);
}

/**
 * cap_table_weight - Count of bits in *srcp
 * @srcp: the cap_table to count bits (< cap_nr_max) in.
 */
static inline unsigned int cap_table_weight(const struct cap_table *srcp)
{
	return bitmap_weight(cap_table_bits(srcp), cap_nr_max);
}

/**
 * cap_table_shift_right - *dstp = *srcp >> n
 * @dstp: the cap_table result
 * @srcp: the input to shift
 * @n: the number of bits to shift by
 */
static inline void cap_table_shift_right(struct cap_table *dstp,
				       const struct cap_table *srcp, int n)
{
	bitmap_shift_right(cap_table_bits(dstp), cap_table_bits(srcp), n,
					       cap_nr_max);
}

/**
 * cap_table_shift_left - *dstp = *srcp << n
 * @dstp: the cap_table result
 * @srcp: the input to shift
 * @n: the number of bits to shift by
 */
static inline void cap_table_shift_left(struct cap_table *dstp,
				      const struct cap_table *srcp, int n)
{
	bitmap_shift_left(cap_table_bits(dstp), cap_table_bits(srcp), n,
					      cap_nr_max);
}

/**
 * cap_table_copy - *dstp = *srcp
 * @dstp: the result
 * @srcp: the input cap_table
 */
static inline void cap_table_copy(struct cap_table *dstp,
				const struct cap_table *srcp)
{
	bitmap_copy(cap_table_bits(dstp), cap_table_bits(srcp), cap_nr_max);
}

/**
 * cap_table_any - pick a "random" cap from *srcp
 * @srcp: the input cap_table
 *
 * Returns >= cap_nr_max if no caps set.
 */
#define cap_table_any(srcp) cap_table_first(srcp)

/**
 * cap_table_first_and - return the first cap from *srcp1 & *srcp2
 * @src1p: the first input
 * @src2p: the second input
 *
 * Returns >= cap_nr_max if no caps set in both.  See also cap_table_next_and().
 */
#define cap_table_first_and(src1p, src2p) cap_table_next_and(-1, (src1p), (src2p))

/**
 * cap_table_any_and - pick a "random" cap from *mask1 & *mask2
 * @mask1: the first input cap_table
 * @mask2: the second input cap_table
 *
 * Returns >= cap_nr_max if no caps set.
 */
#define cap_table_any_and(mask1, mask2) cap_table_first_and((mask1), (mask2))

/**
 * cap_table_size - size to allocate for a 'struct cap_table' in bytes
 */
static inline unsigned int cap_table_size(void)
{
	return BITS_TO_LONGS(cap_nr_max) * sizeof(long);
}


/**
 * for_each_cap_table - iterate over every cap in a mask
 * @cap: the (optionally unsigned) integer iterator
 * @cap_table: the cap_table pointer
 *
 * After the loop, cap is >= cap_nr_max.
 */
#define for_each_cap_table(cap, cap_table)	\
	for ((cap) = -1;				\
		(cap) = cap_table_next((cap), (cap_table)),	\
		(cap) < cap_nr_max;)

/**
 * for_each_cap_table_not - iterate over every cap in a complemented mask
 * @cap: the (optionally unsigned) integer iterator
 * @cap_table: the cap_table pointer
 *
 * After the loop, cap is >= cap_nr_max.
 */
#define for_each_cap_table_not(cap, cap_table)	\
	for ((cap) = -1;				\
		(cap) = cap_table_next_zero((cap), (cap_table)),	\
		(cap) < cap_nr_max;)

#define CAP_TABLE_MASK_NONE						\
{								\
	[0 ... BITS_TO_LONGS(CAP_NR_MAX)-1] = 0UL			\
}

#define CAP_TABLE_MASK_CAP0						\
{								\
	[0] =  1UL						\
}

#endif /* !__SEL4M_OBJECT_CAP_TYPES_H_ */
