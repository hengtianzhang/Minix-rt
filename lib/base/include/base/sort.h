/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_SORT_H_
#define __BASE_SORT_H_

#include <base/types.h>

void sort(void *base, size_t num, size_t size,
	  int (*cmp)(const void *, const void *),
	  void (*swap)(void *, void *, int));

#endif /* !__BASE_SORT_H_ */
