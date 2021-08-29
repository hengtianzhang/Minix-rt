#include <sel4m/compiler.h>
#include <sel4m/linkage.h>

asmlinkage __visible int printf(const char *fmt, ...)
{
    return 0;
}
