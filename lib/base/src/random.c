// SPDX-License-Identifier: GPL-2.0+
/*
 * Simple xorshift PRNG
 *   see http://www.jstatsoft.org/v08/i14/paper
 *
 * Copyright (c) 2012 Michael Walle
 * Michael Walle <michael@walle.cc>
 */
#include <base/compiler.h>
#include <base/random.h>

static u64 y = 1;

static u64 random_r(u64 *seedp)
{
	*seedp ^= (*seedp << 13);
	*seedp ^= (*seedp >> 17);
	*seedp ^= (*seedp << 5);

	return *seedp;
}

u64 random(void)
{
	return random_r(&y);
}

u64 random_range(u64 min, u64 max)
{
	u64 temp;

	if (unlikely(min > max))
		return 0;

	if (unlikely(min == max))
		return min;

	temp = max - min + 1;

	return min + (random() % temp);
}

void srand(u64 seed)
{
	y = seed;
}
