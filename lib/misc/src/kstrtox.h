/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MISC_SRC_KSTRTOX_H_
#define __MISC_SRC_KSTRTOX_H_

#define KSTRTOX_OVERFLOW	(1U << 31)
const char *_parse_integer_fixup_radix(const char *s, unsigned int *base);
unsigned int _parse_integer(const char *s, unsigned int base, unsigned long long *res);

#endif /* !__MISC_SRC_KSTRTOX_H_ */
