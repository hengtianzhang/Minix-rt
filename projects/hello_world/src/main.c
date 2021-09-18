#include <stdio.h>

int main(void)
{
	asm volatile (
		"mov x8, -2\n\t"
		"mov x1, 3\n\t"
		"svc #0x0\n\t"
		);

	printf("sssssssssssssssssss\n");

	while(1);

	return 0;
}
