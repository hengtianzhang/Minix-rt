#include <base/linkage.h>

ENTRY(_start)
	bl	main
1:
	b	1b
ENDPROC(_start)
