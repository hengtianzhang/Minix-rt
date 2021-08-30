#include <sel4m/compiler.h>
#include <sel4m/linkage.h>

#ifdef CONFIG_STACKPROTECTOR

/*
 * Called when gcc's -fstack-protector feature is used, and
 * gcc detects corruption of the on-stack canary value
 */
__visible void __stack_chk_fail(void)
{
}

#endif

/**
 *	panic - halt the system
 *	@fmt: The text string to print
 *
 *	Display a message, then perform cleanups.
 *
 *	This function never returns.
 */
void panic(const char *fmt, ...)
{
}

/**
 *	panic - halt the system
 *	@fmt: The text string to print
 *
 *	Display a message, then perform cleanups.
 *
 *	This function never returns.
 */
void hang(const char *fmt, ...)
{
}
