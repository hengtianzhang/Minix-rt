#ifndef __MINIX_RT_RANDOM_H_
#define __MINIX_RT_RANDOM_H_

#define RAND_MAX -1ULL

/*
 * On 64-bit architectures, protect against non-terminated C string overflows
 * by zeroing out the first byte of the canary; this leaves 56 bits of entropy.
 */
#ifdef CONFIG_64BIT
# ifdef __LITTLE_ENDIAN
#  define CANARY_MASK 0xffffffffffffff00UL
# else /* big endian, 64 bits: */
#  define CANARY_MASK 0x00ffffffffffffffUL
# endif
#else /* 32 bits: */
# define CANARY_MASK 0xffffffffUL
#endif

void srand(u64 seed);
u64 random(void);
u64 random_range(u64 min, u64 max);

static inline u64 random_max(u64 max)
{
	return random_range(0, max);
}

static inline u64 get_random_canary(void)
{
	u64 val = random();

	return val & CANARY_MASK;
}

#endif /* !__MINIX_RT_RANDOM_H_ */
