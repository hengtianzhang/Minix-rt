/*
 * lib/bitmap.c
 * Helper functions for bitmap.h.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */
#include <misc/ctype.h>
#include <misc/errno.h>
#include <misc/bitmap.h>
#include <misc/bitops.h>
#include <misc/common.h>
#include <misc/string.h>


/**
 * DOC: bitmap introduction
 *
 * bitmaps provide an array of bits, implemented using an an
 * array of unsigned longs.  The number of valid bits in a
 * given bitmap does _not_ need to be an exact multiple of
 * BITS_PER_LONG.
 *
 * The possible unused bits in the last, partially used word
 * of a bitmap are 'don't care'.  The implementation makes
 * no particular effort to keep them zero.  It ensures that
 * their value will not affect the results of any operation.
 * The bitmap operations that return Boolean (bitmap_empty,
 * for example) or scalar (bitmap_weight, for example) results
 * carefully filter out these unused bits from impacting their
 * results.
 *
 * The byte ordering of bitmaps is more natural on little
 * endian architectures.  See the big-endian headers
 * include/asm-ppc64/bitops.h and include/asm-s390/bitops.h
 * for the best explanations of this ordering.
 */

int __bitmap_equal(const unsigned long *bitmap1,
		const unsigned long *bitmap2, unsigned int bits)
{
	unsigned int k, lim = bits/BITS_PER_LONG;
	for (k = 0; k < lim; ++k)
		if (bitmap1[k] != bitmap2[k])
			return 0;

	if (bits % BITS_PER_LONG)
		if ((bitmap1[k] ^ bitmap2[k]) & BITMAP_LAST_WORD_MASK(bits))
			return 0;

	return 1;
}

void __bitmap_complement(unsigned long *dst, const unsigned long *src, unsigned int bits)
{
	unsigned int k, lim = BITS_TO_LONGS(bits);
	for (k = 0; k < lim; ++k)
		dst[k] = ~src[k];
}

/**
 * __bitmap_shift_right - logical right shift of the bits in a bitmap
 *   @dst : destination bitmap
 *   @src : source bitmap
 *   @shift : shift by this many bits
 *   @nbits : bitmap size, in bits
 *
 * Shifting right (dividing) means moving bits in the MS -> LS bit
 * direction.  Zeros are fed into the vacated MS positions and the
 * LS bits shifted off the bottom are lost.
 */
void __bitmap_shift_right(unsigned long *dst, const unsigned long *src,
			unsigned shift, unsigned nbits)
{
	unsigned k, lim = BITS_TO_LONGS(nbits);
	unsigned off = shift/BITS_PER_LONG, rem = shift % BITS_PER_LONG;
	unsigned long mask = BITMAP_LAST_WORD_MASK(nbits);
	for (k = 0; off + k < lim; ++k) {
		unsigned long upper, lower;

		/*
		 * If shift is not word aligned, take lower rem bits of
		 * word above and make them the top rem bits of result.
		 */
		if (!rem || off + k + 1 >= lim)
			upper = 0;
		else {
			upper = src[off + k + 1];
			if (off + k + 1 == lim - 1)
				upper &= mask;
			upper <<= (BITS_PER_LONG - rem);
		}
		lower = src[off + k];
		if (off + k == lim - 1)
			lower &= mask;
		lower >>= rem;
		dst[k] = lower | upper;
	}
	if (off)
		memset(&dst[lim - off], 0, off*sizeof(unsigned long));
}

/**
 * __bitmap_shift_left - logical left shift of the bits in a bitmap
 *   @dst : destination bitmap
 *   @src : source bitmap
 *   @shift : shift by this many bits
 *   @nbits : bitmap size, in bits
 *
 * Shifting left (multiplying) means moving bits in the LS -> MS
 * direction.  Zeros are fed into the vacated LS bit positions
 * and those MS bits shifted off the top are lost.
 */

void __bitmap_shift_left(unsigned long *dst, const unsigned long *src,
			unsigned int shift, unsigned int nbits)
{
	int k;
	unsigned int lim = BITS_TO_LONGS(nbits);
	unsigned int off = shift/BITS_PER_LONG, rem = shift % BITS_PER_LONG;
	for (k = lim - off - 1; k >= 0; --k) {
		unsigned long upper, lower;

		/*
		 * If shift is not word aligned, take upper rem bits of
		 * word below and make them the bottom rem bits of result.
		 */
		if (rem && k > 0)
			lower = src[k - 1] >> (BITS_PER_LONG - rem);
		else
			lower = 0;
		upper = src[k] << rem;
		dst[k + off] = lower | upper;
	}
	if (off)
		memset(dst, 0, off*sizeof(unsigned long));
}

int __bitmap_and(unsigned long *dst, const unsigned long *bitmap1,
				const unsigned long *bitmap2, unsigned int bits)
{
	unsigned int k;
	unsigned int lim = bits/BITS_PER_LONG;
	unsigned long result = 0;

	for (k = 0; k < lim; k++)
		result |= (dst[k] = bitmap1[k] & bitmap2[k]);
	if (bits % BITS_PER_LONG)
		result |= (dst[k] = bitmap1[k] & bitmap2[k] &
			   BITMAP_LAST_WORD_MASK(bits));
	return result != 0;
}

void __bitmap_or(unsigned long *dst, const unsigned long *bitmap1,
				const unsigned long *bitmap2, unsigned int bits)
{
	unsigned int k;
	unsigned int nr = BITS_TO_LONGS(bits);

	for (k = 0; k < nr; k++)
		dst[k] = bitmap1[k] | bitmap2[k];
}

void __bitmap_xor(unsigned long *dst, const unsigned long *bitmap1,
				const unsigned long *bitmap2, unsigned int bits)
{
	unsigned int k;
	unsigned int nr = BITS_TO_LONGS(bits);

	for (k = 0; k < nr; k++)
		dst[k] = bitmap1[k] ^ bitmap2[k];
}

int __bitmap_andnot(unsigned long *dst, const unsigned long *bitmap1,
				const unsigned long *bitmap2, unsigned int bits)
{
	unsigned int k;
	unsigned int lim = bits/BITS_PER_LONG;
	unsigned long result = 0;

	for (k = 0; k < lim; k++)
		result |= (dst[k] = bitmap1[k] & ~bitmap2[k]);
	if (bits % BITS_PER_LONG)
		result |= (dst[k] = bitmap1[k] & ~bitmap2[k] &
			   BITMAP_LAST_WORD_MASK(bits));
	return result != 0;
}

int __bitmap_intersects(const unsigned long *bitmap1,
			const unsigned long *bitmap2, unsigned int bits)
{
	unsigned int k, lim = bits/BITS_PER_LONG;
	for (k = 0; k < lim; ++k)
		if (bitmap1[k] & bitmap2[k])
			return 1;

	if (bits % BITS_PER_LONG)
		if ((bitmap1[k] & bitmap2[k]) & BITMAP_LAST_WORD_MASK(bits))
			return 1;
	return 0;
}

int __bitmap_subset(const unsigned long *bitmap1,
		    const unsigned long *bitmap2, unsigned int bits)
{
	unsigned int k, lim = bits/BITS_PER_LONG;
	for (k = 0; k < lim; ++k)
		if (bitmap1[k] & ~bitmap2[k])
			return 0;

	if (bits % BITS_PER_LONG)
		if ((bitmap1[k] & ~bitmap2[k]) & BITMAP_LAST_WORD_MASK(bits))
			return 0;
	return 1;
}

int __bitmap_weight(const unsigned long *bitmap, unsigned int bits)
{
	unsigned int k, lim = bits/BITS_PER_LONG;
	int w = 0;

	for (k = 0; k < lim; k++)
		w += hweight_long(bitmap[k]);

	if (bits % BITS_PER_LONG)
		w += hweight_long(bitmap[k] & BITMAP_LAST_WORD_MASK(bits));

	return w;
}

void __bitmap_set(unsigned long *map, unsigned int start, int len)
{
	unsigned long *p = map + BIT_WORD(start);
	const unsigned int size = start + len;
	int bits_to_set = BITS_PER_LONG - (start % BITS_PER_LONG);
	unsigned long mask_to_set = BITMAP_FIRST_WORD_MASK(start);

	while (len - bits_to_set >= 0) {
		*p |= mask_to_set;
		len -= bits_to_set;
		bits_to_set = BITS_PER_LONG;
		mask_to_set = ~0UL;
		p++;
	}
	if (len) {
		mask_to_set &= BITMAP_LAST_WORD_MASK(size);
		*p |= mask_to_set;
	}
}

void __bitmap_clear(unsigned long *map, unsigned int start, int len)
{
	unsigned long *p = map + BIT_WORD(start);
	const unsigned int size = start + len;
	int bits_to_clear = BITS_PER_LONG - (start % BITS_PER_LONG);
	unsigned long mask_to_clear = BITMAP_FIRST_WORD_MASK(start);

	while (len - bits_to_clear >= 0) {
		*p &= ~mask_to_clear;
		len -= bits_to_clear;
		bits_to_clear = BITS_PER_LONG;
		mask_to_clear = ~0UL;
		p++;
	}
	if (len) {
		mask_to_clear &= BITMAP_LAST_WORD_MASK(size);
		*p &= ~mask_to_clear;
	}
}

/**
 * bitmap_find_next_zero_area_off - find a contiguous aligned zero area
 * @map: The address to base the search on
 * @size: The bitmap size in bits
 * @start: The bitnumber to start searching at
 * @nr: The number of zeroed bits we're looking for
 * @align_mask: Alignment mask for zero area
 * @align_offset: Alignment offset for zero area.
 *
 * The @align_mask should be one less than a power of 2; the effect is that
 * the bit offset of all zero areas this function finds plus @align_offset
 * is multiple of that power of 2.
 */
unsigned long bitmap_find_next_zero_area_off(unsigned long *map,
					     unsigned long size,
					     unsigned long start,
					     unsigned int nr,
					     unsigned long align_mask,
					     unsigned long align_offset)
{
	unsigned long index, end, i;
again:
	index = find_next_zero_bit(map, size, start);

	/* Align allocation */
	index = __ALIGN_MASK(index + align_offset, align_mask) - align_offset;

	end = index + nr;
	if (end > size)
		return end;
	i = find_next_bit(map, end, index);
	if (i < end) {
		start = i + 1;
		goto again;
	}
	return index;
}

/*
 * Bitmap printing & parsing functions: first version by Nadia Yvette Chambers,
 * second version by Paul Jackson, third by Joe Korty.
 */

#define CHUNKSZ				32
#define nbits_to_hold_value(val)	fls(val)
#define BASEDEC 10		/* fancier cpuset lists input in decimal */

/**
 * bitmap_pos_to_ord - find ordinal of set bit at given position in bitmap
 *	@buf: pointer to a bitmap
 *	@pos: a bit position in @buf (0 <= @pos < @nbits)
 *	@nbits: number of valid bit positions in @buf
 *
 * Map the bit at position @pos in @buf (of length @nbits) to the
 * ordinal of which set bit it is.  If it is not set or if @pos
 * is not a valid bit position, map to -1.
 *
 * If for example, just bits 4 through 7 are set in @buf, then @pos
 * values 4 through 7 will get mapped to 0 through 3, respectively,
 * and other @pos values will get mapped to -1.  When @pos value 7
 * gets mapped to (returns) @ord value 3 in this example, that means
 * that bit 7 is the 3rd (starting with 0th) set bit in @buf.
 *
 * The bit positions 0 through @bits are valid positions in @buf.
 */
static int bitmap_pos_to_ord(const unsigned long *buf, unsigned int pos, unsigned int nbits)
{
	if (pos >= nbits || !test_bit(pos, buf))
		return -1;

	return __bitmap_weight(buf, pos);
}

/**
 * bitmap_ord_to_pos - find position of n-th set bit in bitmap
 *	@buf: pointer to bitmap
 *	@ord: ordinal bit position (n-th set bit, n >= 0)
 *	@nbits: number of valid bit positions in @buf
 *
 * Map the ordinal offset of bit @ord in @buf to its position in @buf.
 * Value of @ord should be in range 0 <= @ord < weight(buf). If @ord
 * >= weight(buf), returns @nbits.
 *
 * If for example, just bits 4 through 7 are set in @buf, then @ord
 * values 0 through 3 will get mapped to 4 through 7, respectively,
 * and all other @ord values returns @nbits.  When @ord value 3
 * gets mapped to (returns) @pos value 7 in this example, that means
 * that the 3rd set bit (starting with 0th) is at position 7 in @buf.
 *
 * The bit positions 0 through @nbits-1 are valid positions in @buf.
 */
unsigned int bitmap_ord_to_pos(const unsigned long *buf, unsigned int ord, unsigned int nbits)
{
	unsigned int pos;

	for (pos = find_first_bit(buf, nbits);
	     pos < nbits && ord;
	     pos = find_next_bit(buf, nbits, pos + 1))
		ord--;

	return pos;
}

/**
 * bitmap_remap - Apply map defined by a pair of bitmaps to another bitmap
 *	@dst: remapped result
 *	@src: subset to be remapped
 *	@old: defines domain of map
 *	@new: defines range of map
 *	@nbits: number of bits in each of these bitmaps
 *
 * Let @old and @new define a mapping of bit positions, such that
 * whatever position is held by the n-th set bit in @old is mapped
 * to the n-th set bit in @new.  In the more general case, allowing
 * for the possibility that the weight 'w' of @new is less than the
 * weight of @old, map the position of the n-th set bit in @old to
 * the position of the m-th set bit in @new, where m == n % w.
 *
 * If either of the @old and @new bitmaps are empty, or if @src and
 * @dst point to the same location, then this routine copies @src
 * to @dst.
 *
 * The positions of unset bits in @old are mapped to themselves
 * (the identify map).
 *
 * Apply the above specified mapping to @src, placing the result in
 * @dst, clearing any bits previously set in @dst.
 *
 * For example, lets say that @old has bits 4 through 7 set, and
 * @new has bits 12 through 15 set.  This defines the mapping of bit
 * position 4 to 12, 5 to 13, 6 to 14 and 7 to 15, and of all other
 * bit positions unchanged.  So if say @src comes into this routine
 * with bits 1, 5 and 7 set, then @dst should leave with bits 1,
 * 13 and 15 set.
 */
void bitmap_remap(unsigned long *dst, const unsigned long *src,
		const unsigned long *old, const unsigned long *new,
		unsigned int nbits)
{
	unsigned int oldbit, w;

	if (dst == src)		/* following doesn't handle inplace remaps */
		return;
	bitmap_zero(dst, nbits);

	w = bitmap_weight(new, nbits);
	for_each_set_bit(oldbit, src, nbits) {
		int n = bitmap_pos_to_ord(old, oldbit, nbits);

		if (n < 0 || w == 0)
			set_bit(oldbit, dst);	/* identity map */
		else
			set_bit(bitmap_ord_to_pos(new, n % w, nbits), dst);
	}
}

/**
 * bitmap_bitremap - Apply map defined by a pair of bitmaps to a single bit
 *	@oldbit: bit position to be mapped
 *	@old: defines domain of map
 *	@new: defines range of map
 *	@bits: number of bits in each of these bitmaps
 *
 * Let @old and @new define a mapping of bit positions, such that
 * whatever position is held by the n-th set bit in @old is mapped
 * to the n-th set bit in @new.  In the more general case, allowing
 * for the possibility that the weight 'w' of @new is less than the
 * weight of @old, map the position of the n-th set bit in @old to
 * the position of the m-th set bit in @new, where m == n % w.
 *
 * The positions of unset bits in @old are mapped to themselves
 * (the identify map).
 *
 * Apply the above specified mapping to bit position @oldbit, returning
 * the new bit position.
 *
 * For example, lets say that @old has bits 4 through 7 set, and
 * @new has bits 12 through 15 set.  This defines the mapping of bit
 * position 4 to 12, 5 to 13, 6 to 14 and 7 to 15, and of all other
 * bit positions unchanged.  So if say @oldbit is 5, then this routine
 * returns 13.
 */
int bitmap_bitremap(int oldbit, const unsigned long *old,
				const unsigned long *new, int bits)
{
	int w = bitmap_weight(new, bits);
	int n = bitmap_pos_to_ord(old, oldbit, bits);
	if (n < 0 || w == 0)
		return oldbit;
	else
		return bitmap_ord_to_pos(new, n % w, bits);
}

/**
 * bitmap_onto - translate one bitmap relative to another
 *	@dst: resulting translated bitmap
 * 	@orig: original untranslated bitmap
 * 	@relmap: bitmap relative to which translated
 *	@bits: number of bits in each of these bitmaps
 *
 * Set the n-th bit of @dst iff there exists some m such that the
 * n-th bit of @relmap is set, the m-th bit of @orig is set, and
 * the n-th bit of @relmap is also the m-th _set_ bit of @relmap.
 * (If you understood the previous sentence the first time your
 * read it, you're overqualified for your current job.)
 *
 * In other words, @orig is mapped onto (surjectively) @dst,
 * using the map { <n, m> | the n-th bit of @relmap is the
 * m-th set bit of @relmap }.
 *
 * Any set bits in @orig above bit number W, where W is the
 * weight of (number of set bits in) @relmap are mapped nowhere.
 * In particular, if for all bits m set in @orig, m >= W, then
 * @dst will end up empty.  In situations where the possibility
 * of such an empty result is not desired, one way to avoid it is
 * to use the bitmap_fold() operator, below, to first fold the
 * @orig bitmap over itself so that all its set bits x are in the
 * range 0 <= x < W.  The bitmap_fold() operator does this by
 * setting the bit (m % W) in @dst, for each bit (m) set in @orig.
 *
 * Example [1] for bitmap_onto():
 *  Let's say @relmap has bits 30-39 set, and @orig has bits
 *  1, 3, 5, 7, 9 and 11 set.  Then on return from this routine,
 *  @dst will have bits 31, 33, 35, 37 and 39 set.
 *
 *  When bit 0 is set in @orig, it means turn on the bit in
 *  @dst corresponding to whatever is the first bit (if any)
 *  that is turned on in @relmap.  Since bit 0 was off in the
 *  above example, we leave off that bit (bit 30) in @dst.
 *
 *  When bit 1 is set in @orig (as in the above example), it
 *  means turn on the bit in @dst corresponding to whatever
 *  is the second bit that is turned on in @relmap.  The second
 *  bit in @relmap that was turned on in the above example was
 *  bit 31, so we turned on bit 31 in @dst.
 *
 *  Similarly, we turned on bits 33, 35, 37 and 39 in @dst,
 *  because they were the 4th, 6th, 8th and 10th set bits
 *  set in @relmap, and the 4th, 6th, 8th and 10th bits of
 *  @orig (i.e. bits 3, 5, 7 and 9) were also set.
 *
 *  When bit 11 is set in @orig, it means turn on the bit in
 *  @dst corresponding to whatever is the twelfth bit that is
 *  turned on in @relmap.  In the above example, there were
 *  only ten bits turned on in @relmap (30..39), so that bit
 *  11 was set in @orig had no affect on @dst.
 *
 * Example [2] for bitmap_fold() + bitmap_onto():
 *  Let's say @relmap has these ten bits set::
 *
 *		40 41 42 43 45 48 53 61 74 95
 *
 *  (for the curious, that's 40 plus the first ten terms of the
 *  Fibonacci sequence.)
 *
 *  Further lets say we use the following code, invoking
 *  bitmap_fold() then bitmap_onto, as suggested above to
 *  avoid the possibility of an empty @dst result::
 *
 *	unsigned long *tmp;	// a temporary bitmap's bits
 *
 *	bitmap_fold(tmp, orig, bitmap_weight(relmap, bits), bits);
 *	bitmap_onto(dst, tmp, relmap, bits);
 *
 *  Then this table shows what various values of @dst would be, for
 *  various @orig's.  I list the zero-based positions of each set bit.
 *  The tmp column shows the intermediate result, as computed by
 *  using bitmap_fold() to fold the @orig bitmap modulo ten
 *  (the weight of @relmap):
 *
 *      =============== ============== =================
 *      @orig           tmp            @dst
 *      0                0             40
 *      1                1             41
 *      9                9             95
 *      10               0             40 [#f1]_
 *      1 3 5 7          1 3 5 7       41 43 48 61
 *      0 1 2 3 4        0 1 2 3 4     40 41 42 43 45
 *      0 9 18 27        0 9 8 7       40 61 74 95
 *      0 10 20 30       0             40
 *      0 11 22 33       0 1 2 3       40 41 42 43
 *      0 12 24 36       0 2 4 6       40 42 45 53
 *      78 102 211       1 2 8         41 42 74 [#f1]_
 *      =============== ============== =================
 *
 * .. [#f1]
 *
 *     For these marked lines, if we hadn't first done bitmap_fold()
 *     into tmp, then the @dst result would have been empty.
 *
 * If either of @orig or @relmap is empty (no set bits), then @dst
 * will be returned empty.
 *
 * If (as explained above) the only set bits in @orig are in positions
 * m where m >= W, (where W is the weight of @relmap) then @dst will
 * once again be returned empty.
 *
 * All bits in @dst not set by the above rule are cleared.
 */
void bitmap_onto(unsigned long *dst, const unsigned long *orig,
			const unsigned long *relmap, unsigned int bits)
{
	unsigned int n, m;	/* same meaning as in above comment */

	if (dst == orig)	/* following doesn't handle inplace mappings */
		return;
	bitmap_zero(dst, bits);

	/*
	 * The following code is a more efficient, but less
	 * obvious, equivalent to the loop:
	 *	for (m = 0; m < bitmap_weight(relmap, bits); m++) {
	 *		n = bitmap_ord_to_pos(orig, m, bits);
	 *		if (test_bit(m, orig))
	 *			set_bit(n, dst);
	 *	}
	 */

	m = 0;
	for_each_set_bit(n, relmap, bits) {
		/* m == bitmap_pos_to_ord(relmap, n, bits) */
		if (test_bit(m, orig))
			set_bit(n, dst);
		m++;
	}
}

/**
 * bitmap_fold - fold larger bitmap into smaller, modulo specified size
 *	@dst: resulting smaller bitmap
 *	@orig: original larger bitmap
 *	@sz: specified size
 *	@nbits: number of bits in each of these bitmaps
 *
 * For each bit oldbit in @orig, set bit oldbit mod @sz in @dst.
 * Clear all other bits in @dst.  See further the comment and
 * Example [2] for bitmap_onto() for why and how to use this.
 */
void bitmap_fold(unsigned long *dst, const unsigned long *orig,
			unsigned int sz, unsigned int nbits)
{
	unsigned int oldbit;

	if (dst == orig)	/* following doesn't handle inplace mappings */
		return;
	bitmap_zero(dst, nbits);

	for_each_set_bit(oldbit, orig, nbits)
		set_bit(oldbit % sz, dst);
}

/*
 * Common code for bitmap_*_region() routines.
 *	bitmap: array of unsigned longs corresponding to the bitmap
 *	pos: the beginning of the region
 *	order: region size (log base 2 of number of bits)
 *	reg_op: operation(s) to perform on that region of bitmap
 *
 * Can set, verify and/or release a region of bits in a bitmap,
 * depending on which combination of REG_OP_* flag bits is set.
 *
 * A region of a bitmap is a sequence of bits in the bitmap, of
 * some size '1 << order' (a power of two), aligned to that same
 * '1 << order' power of two.
 *
 * Returns 1 if REG_OP_ISFREE succeeds (region is all zero bits).
 * Returns 0 in all other cases and reg_ops.
 */

enum {
	REG_OP_ISFREE,		/* true if region is all zero bits */
	REG_OP_ALLOC,		/* set all bits in region */
	REG_OP_RELEASE,		/* clear all bits in region */
};

static int __reg_op(unsigned long *bitmap, unsigned int pos, int order, int reg_op)
{
	int nbits_reg;		/* number of bits in region */
	int index;		/* index first long of region in bitmap */
	int offset;		/* bit offset region in bitmap[index] */
	int nlongs_reg;		/* num longs spanned by region in bitmap */
	int nbitsinlong;	/* num bits of region in each spanned long */
	unsigned long mask;	/* bitmask for one long of region */
	int i;			/* scans bitmap by longs */
	int ret = 0;		/* return value */

	/*
	 * Either nlongs_reg == 1 (for small orders that fit in one long)
	 * or (offset == 0 && mask == ~0UL) (for larger multiword orders.)
	 */
	nbits_reg = 1 << order;
	index = pos / BITS_PER_LONG;
	offset = pos - (index * BITS_PER_LONG);
	nlongs_reg = BITS_TO_LONGS(nbits_reg);
	nbitsinlong = min(nbits_reg,  BITS_PER_LONG);

	/*
	 * Can't do "mask = (1UL << nbitsinlong) - 1", as that
	 * overflows if nbitsinlong == BITS_PER_LONG.
	 */
	mask = (1UL << (nbitsinlong - 1));
	mask += mask - 1;
	mask <<= offset;

	switch (reg_op) {
	case REG_OP_ISFREE:
		for (i = 0; i < nlongs_reg; i++) {
			if (bitmap[index + i] & mask)
				goto done;
		}
		ret = 1;	/* all bits in region free (zero) */
		break;

	case REG_OP_ALLOC:
		for (i = 0; i < nlongs_reg; i++)
			bitmap[index + i] |= mask;
		break;

	case REG_OP_RELEASE:
		for (i = 0; i < nlongs_reg; i++)
			bitmap[index + i] &= ~mask;
		break;
	}
done:
	return ret;
}

/**
 * bitmap_find_free_region - find a contiguous aligned mem region
 *	@bitmap: array of unsigned longs corresponding to the bitmap
 *	@bits: number of bits in the bitmap
 *	@order: region size (log base 2 of number of bits) to find
 *
 * Find a region of free (zero) bits in a @bitmap of @bits bits and
 * allocate them (set them to one).  Only consider regions of length
 * a power (@order) of two, aligned to that power of two, which
 * makes the search algorithm much faster.
 *
 * Return the bit offset in bitmap of the allocated region,
 * or -errno on failure.
 */
int bitmap_find_free_region(unsigned long *bitmap, unsigned int bits, int order)
{
	unsigned int pos, end;		/* scans bitmap by regions of size order */

	for (pos = 0 ; (end = pos + (1U << order)) <= bits; pos = end) {
		if (!__reg_op(bitmap, pos, order, REG_OP_ISFREE))
			continue;
		__reg_op(bitmap, pos, order, REG_OP_ALLOC);
		return pos;
	}
	return -ENOMEM;
}

/**
 * bitmap_release_region - release allocated bitmap region
 *	@bitmap: array of unsigned longs corresponding to the bitmap
 *	@pos: beginning of bit region to release
 *	@order: region size (log base 2 of number of bits) to release
 *
 * This is the complement to __bitmap_find_free_region() and releases
 * the found region (by clearing it in the bitmap).
 *
 * No return value.
 */
void bitmap_release_region(unsigned long *bitmap, unsigned int pos, int order)
{
	__reg_op(bitmap, pos, order, REG_OP_RELEASE);
}

/**
 * bitmap_allocate_region - allocate bitmap region
 *	@bitmap: array of unsigned longs corresponding to the bitmap
 *	@pos: beginning of bit region to allocate
 *	@order: region size (log base 2 of number of bits) to allocate
 *
 * Allocate (set bits in) a specified region of a bitmap.
 *
 * Return 0 on success, or %-EBUSY if specified region wasn't
 * free (not all bits were zero).
 */
int bitmap_allocate_region(unsigned long *bitmap, unsigned int pos, int order)
{
	if (!__reg_op(bitmap, pos, order, REG_OP_ISFREE))
		return -EBUSY;
	return __reg_op(bitmap, pos, order, REG_OP_ALLOC);
}

/**
 * bitmap_copy_le - copy a bitmap, putting the bits into little-endian order.
 * @dst:   destination buffer
 * @src:   bitmap to copy
 * @nbits: number of bits in the bitmap
 *
 * Require nbits % BITS_PER_LONG == 0.
 */
#ifdef __BIG_ENDIAN
void bitmap_copy_le(unsigned long *dst, const unsigned long *src, unsigned int nbits)
{
	unsigned int i;

	for (i = 0; i < nbits/BITS_PER_LONG; i++) {
		if (BITS_PER_LONG == 64)
			dst[i] = cpu_to_le64(src[i]);
		else
			dst[i] = cpu_to_le32(src[i]);
	}
}
#endif

#if BITS_PER_LONG == 64
/**
 * bitmap_from_arr32 - copy the contents of u32 array of bits to bitmap
 *	@bitmap: array of unsigned longs, the destination bitmap
 *	@buf: array of u32 (in host byte order), the source bitmap
 *	@nbits: number of bits in @bitmap
 */
void bitmap_from_arr32(unsigned long *bitmap, const u32 *buf, unsigned int nbits)
{
	unsigned int i, halfwords;

	halfwords = DIV_ROUND_UP(nbits, 32);
	for (i = 0; i < halfwords; i++) {
		bitmap[i/2] = (unsigned long) buf[i];
		if (++i < halfwords)
			bitmap[i/2] |= ((unsigned long) buf[i]) << 32;
	}

	/* Clear tail bits in last word beyond nbits. */
	if (nbits % BITS_PER_LONG)
		bitmap[(halfwords - 1) / 2] &= BITMAP_LAST_WORD_MASK(nbits);
}

/**
 * bitmap_to_arr32 - copy the contents of bitmap to a u32 array of bits
 *	@buf: array of u32 (in host byte order), the dest bitmap
 *	@bitmap: array of unsigned longs, the source bitmap
 *	@nbits: number of bits in @bitmap
 */
void bitmap_to_arr32(u32 *buf, const unsigned long *bitmap, unsigned int nbits)
{
	unsigned int i, halfwords;

	halfwords = DIV_ROUND_UP(nbits, 32);
	for (i = 0; i < halfwords; i++) {
		buf[i] = (u32) (bitmap[i/2] & UINT_MAX);
		if (++i < halfwords)
			buf[i] = (u32) (bitmap[i/2] >> 32);
	}

	/* Clear tail bits in last element of array beyond nbits. */
	if (nbits % BITS_PER_LONG)
		buf[halfwords - 1] &= (u32) (UINT_MAX >> ((-nbits) & 31));
}
#endif
