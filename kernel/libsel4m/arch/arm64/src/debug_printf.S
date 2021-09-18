#include <base/linkage.h>
#include <base/errno.h>

#include <asm/unistd.h>

ENTRY(__debug_printf)
	mov		x8,	#__NR_debug_printf
	sxtw	x1, w1
	svc		#0
	cmn		x0,	#MAX_ERRNO
	ret
ENDPROC(__debug_printf)
