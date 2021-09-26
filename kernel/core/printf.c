#include <base/linkage.h>
#include <base/compiler.h>
#include <base/common.h>
#include <base/errno.h>

#include <sel4m/object/ipc.h>
#include <sel4m/sched/clock.h>
#include <sel4m/spinlock.h>
#include <sel4m/syscalls.h>
#include <sel4m/uaccess.h>

unsigned long long __stack_chk_guard;

static DEFINE_RAW_SPINLOCK(printk_lock);

#define EXT_LINE_MAX	32

static inline int printf_get_level(const char *buffer)
{
	if (buffer[0] == KERN_SOH_ASCII && buffer[1]) {
		switch (buffer[1]) {
		case '0' ... '7':
		case 'd':	/* KERN_DEFAULT */
		case 'c':	/* KERN_CONT */
			return buffer[1];
		}
	}
	return 0;
}

static inline const char *printf_skip_level(const char *buffer)
{
	if (printf_get_level(buffer))
		return buffer + 2;

	return buffer;
}

static size_t print_time(u64 ts, char *buf)
{
	u64 rem_nsec = do_div(ts, 1000000000);

	return sprintf(buf, "[%5llu.%06llu] ",
		       (u64)ts, rem_nsec / 1000);
}

extern void puts_q(const char *str);
asmlinkage int vprintf_emit(int facility, int level,
			    const char *dict, size_t dictlen,
			    const char *fmt, va_list args)
{
	static char textbuf[1024];
	char extbuf[EXT_LINE_MAX];
	char *text = textbuf;
	int skip_time = 0;
	size_t text_len = 0;
	u64 flags;

	/* This stops the holder of console_sem just where we want him */
	raw_spin_lock_irqsave(&printk_lock, flags);
	/*
	 * The printf needs to come first; we need the syslog
	 * prefix which might be passed-in as a parameter.
	 */
	text_len = vscnprintf(text, sizeof(textbuf), fmt, args);

	if (text_len && text[text_len - 1] == '\n')
		text_len--;

	/* strip kernel syslog prefix and extract log level or control flags */
	if (facility == 0) {
		int kern_level = printf_get_level(text);

		if (kern_level) {
			const char *end_of_header = printf_skip_level(text);
			switch (kern_level) {
				case 'c':
					skip_time = 1;
			}
			/*
			 * No need to check length here because vscnprintf
			 * put '\0' at the end of the string. Only valid and
			 * newly printed level is detected.
			 */
			text_len -= end_of_header - text;
			text = (char *)end_of_header;
		}
	}

	if (likely(!skip_time)) {
		print_time(local_clock(), extbuf);
		puts_q(extbuf);
	}
    puts_q(text);

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

SYSCALL_DEFINE2(debug_printf, const char __user *, ptr, int, len)
{
	u64 flags;
//	char put_buffer[IPC_DEBUG_PRINTF_BUFFER_MAX] = {0};
	char *buffer;

	if (len > IPC_DEBUG_PRINTF_BUFFER_MAX)
		return -EMSGSIZE;

// Now, slowpath.
//	if(copy_from_user(put_buffer, ptr, len))
//		return -EFAULT;

	buffer = ipc_get_debug_buffer();
	buffer[IPC_DEBUG_PRINTF_BUFFER_MAX - 1] = '\0';
	/* This stops the holder of console_sem just where we want him */
	raw_spin_lock_irqsave(&printk_lock, flags);
	//puts_q(put_buffer);
	puts_q(ipc_get_debug_buffer());
	raw_spin_unlock_irqrestore(&printk_lock, flags);

	return 0;
}
