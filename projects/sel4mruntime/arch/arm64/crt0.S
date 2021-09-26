#include <base/linkage.h>

ENTRY(_start)
	sub	sp, sp, #8
	bl	__sel4m_start_c
	bl	main
	b	__sel4m_exit_c
ENDPROC(_start)
