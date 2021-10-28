#ifndef __UAPI_ASM_GENERIC_SIGNAL_H_
#define __UAPI_ASM_GENERIC_SIGNAL_H_

#define _NSIG		64
#define _NSIG_BPW	__BITS_PER_LONG
#define _NSIG_WORDS	(_NSIG / _NSIG_BPW)

#ifndef __ASSEMBLY__
typedef struct {
	unsigned long sig[_NSIG_WORDS];
} sigset_t;
#endif /* !__ASSEMBLY__ */

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
/*
#define SIGLOST		29
*/
#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31

/* These should not be considered constants from userland.  */
#define SIGRTMIN	32
#define SIGRTMAX	_NSIG

#ifndef __ASSEMBLY__

#define SI_MAX_SIZE	128

#ifndef __ARCH_SI_ATTRIBUTES
#define __ARCH_SI_ATTRIBUTES
#endif

union __sifields {
	struct {
		pid_t pid;
	} _kill;
};

#ifndef __ARCH_HAS_SWAPPED_SIGINFO
#define __SIGINFO 			\
struct {				\
	int si_signo;			\
	int si_errno;			\
	int si_code;			\
	union __sifields _sifields;	\
}
#else
#define __SIGINFO 			\
struct {				\
	int si_signo;			\
	int si_code;			\
	int si_errno;			\
	union __sifields _sifields;	\
}
#endif /* __ARCH_HAS_SWAPPED_SIGINFO */

typedef struct siginfo {
	union {
		__SIGINFO;
		int _si_pad[SI_MAX_SIZE/sizeof(int)];
	};
} __ARCH_SI_ATTRIBUTES siginfo_t;
#endif

#endif /* !__UAPI_ASM_GENERIC_SIGNAL_H_ */
