/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MISC_BITMAP_H_
#define __MISC_BITMAP_H_

#ifndef __ASSEMBLY__

#define DECLARE_BITMAP(name,bits) \
	u64 name[BITS_TO_LONGS(bits)]

#endif /* !__ASSEMBLY__ */
#endif /* !__MISC_BITMAP_H_ */
