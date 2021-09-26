/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __SEL4M_SIGNAL_H_
#define __SEL4M_SIGNAL_H_

#include <base/types.h>

#include <sel4m/object/notifier.h>

#include <asm/siginfo.h>

extern int do_send_signal(int signal, pid_t pid, long private);

extern bool get_signal(struct ksignal *ksig);

extern void signal_setup_done(int failed, struct ksignal *ksig, int stepping);

#endif /* !__SEL4M_SIGNAL_H_ */
