#include <base/linkage.h>

ENTRY(_start)
	bl	__sel4m_start_c
	bl	main
1:
	b	1b
ENDPROC(_start)
