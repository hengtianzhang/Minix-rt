#include <base/linkage.h>
#include <base/compiler.h>
#include <base/common.h>

unsigned long long __stack_chk_guard;

extern void puts_q(const char *str);
asmlinkage int vprintf_emit(int facility, int level,
			    const char *dict, size_t dictlen,
			    const char *fmt, va_list args)
{
	static char textbuf[1024];
	char *text = textbuf;
	size_t text_len = 0;

	/*
	 * The printf needs to come first; we need the syslog
	 * prefix which might be passed-in as a parameter.
	 */
	text_len = vscnprintf(text, sizeof(textbuf), fmt, args);

    puts_q(textbuf);

    return text_len;
}

asmlinkage __visible int printf(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintf_emit(0, -1, NULL, 0, fmt, args);
	va_end(args);

    return r;
}
