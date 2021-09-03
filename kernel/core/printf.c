#include <base/linkage.h>
#include <base/compiler.h>
#include <base/common.h>

#include <sel4m/spinlock.h>

unsigned long long __stack_chk_guard;

static DEFINE_RAW_SPINLOCK(printk_lock);

extern void puts_q(const char *str);
asmlinkage int vprintf_emit(int facility, int level,
			    const char *dict, size_t dictlen,
			    const char *fmt, va_list args)
{
	static char textbuf[1024];
	char *text = textbuf;
	size_t text_len = 0;
	u64 flags;

	/* This stops the holder of console_sem just where we want him */
	raw_spin_lock_irqsave(&printk_lock, flags);
	/*
	 * The printf needs to come first; we need the syslog
	 * prefix which might be passed-in as a parameter.
	 */
	text_len = vscnprintf(text, sizeof(textbuf), fmt, args);

    puts_q(textbuf);

	raw_spin_unlock_irqrestore(&printk_lock, flags);

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
