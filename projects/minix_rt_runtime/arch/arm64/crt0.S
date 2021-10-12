#include <base/linkage.h>

ENTRY(_start)
	sub	sp, sp, #8
	bl	__minix_rt_start_c
	bl	main
ENDPROC(_start)
