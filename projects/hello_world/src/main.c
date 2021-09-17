
#include <memalloc/memblock.h>
extern int test(int aa);
int test(int aa)
{
	if (aa == 3)
		return 6;

	return aa * 3;
}

int aaa = 123;
unsigned long bbb[46];
char *buf = "Hello World!";

unsigned long const aaaa = 54;

int main(void)
{
	int a;
	bbb[45] = 3;
	aaa += bbb[45];

	a = test(aaa);
	return a;
}
