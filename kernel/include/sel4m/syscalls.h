/*
 * syscalls.h - Linux syscall interfaces (non-arch-specific)
 *
 * Copyright (c) 2004 Randy Dunlap
 * Copyright (c) 2004 Open Source Development Labs
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#ifndef __SEL4M_SYSCALLS_H_
#define __SEL4M_SYSCALLS_H_

#include <sel4m/thread.h>
#include <sel4m/syscalls.h>
#include <sel4m/uaccess.h>

/*
 * Called before coming back to user-mode. Returning to user-mode with an
 * address limit different than USER_DS can allow to overwrite kernel memory.
 */
static inline void addr_limit_user_check(void)
{
#ifdef TIF_FSCHECK
	if (!test_thread_flag(TIF_FSCHECK))
		return;
#endif

	if (CHECK_DATA_CORRUPTION(!segment_eq(get_fs(), USER_DS),
				  "Invalid address limit on user-mode return"))
		//force_sig(SIGKILL, current);
        ; // TODO

#ifdef TIF_FSCHECK
	clear_thread_flag(TIF_FSCHECK);
#endif
}

#endif /* !__SEL4M_SYSCALLS_H_ */
