#include <base/linkage.h>
#include <base/compiler.h>
#include <base/common.h>

#include <libsel4m/syscalls.h>

asmlinkage __visible int printf(const char *fmt, ...)
{
	va_list args;
   char textbuf[1024];
   char *text = textbuf;
   size_t text_len;
	int r = 0;

	va_start(args, fmt);
   text_len = vscnprintf(text, sizeof(textbuf), fmt, args);
   if (text_len) {
      r = __syscall(__NR_debug_printf, text, text_len);
   }
	va_end(args);

    return r;
}
