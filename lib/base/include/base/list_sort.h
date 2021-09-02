/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_LIST_SORT_H_
#define __BASE_LIST_SORT_H_

#include <base/types.h>

struct list_head;

void list_sort(void *priv, struct list_head *head,
	       int (*cmp)(void *priv, struct list_head *a,
			  struct list_head *b));
#endif /* !__BASE_LIST_SORT_H_ */
