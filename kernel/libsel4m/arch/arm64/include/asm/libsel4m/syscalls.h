#ifndef __LIBMINIX_RT_ASM_LIBMINIX_RT_H_
#define __LIBMINIX_RT_ASM_LIBMINIX_RT_H_

#ifndef __scc
#define __scc(X) ((long) (X))
typedef long syscall_arg_t;
#endif

long ___syscall(syscall_arg_t, ...);

#define __syscall0(n) (___syscall)(n,0,0,0,0,0,0)
#define __syscall1(n,a) (___syscall)(n,__scc(a),0,0,0,0,0)
#define __syscall2(n,a,b) (___syscall)(n,__scc(a),__scc(b),0,0,0,0)
#define __syscall3(n,a,b,c) (___syscall)(n,__scc(a),__scc(b),__scc(c),0,0,0)
#define __syscall4(n,a,b,c,d) (___syscall)(n,__scc(a),__scc(b),__scc(c),__scc(d),0,0)
#define __syscall5(n,a,b,c,d,e) (___syscall)(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e),0)
#define __syscall6(n,a,b,c,d,e,f) (___syscall)(n,__scc(a),__scc(b),__scc(c),__scc(d),__scc(e),__scc(f))

#define __SYSCALL_NARGS_X(a,b,c,d,e,f,g,h,n,...) n
#define __SYSCALL_NARGS(...) __SYSCALL_NARGS_X(__VA_ARGS__,7,6,5,4,3,2,1,0,)
#define __SYSCALL_CONCAT_X(a,b) a##b
#define __SYSCALL_CONCAT(a,b) __SYSCALL_CONCAT_X(a,b)
#define __SYSCALL_DISP(b,...) __SYSCALL_CONCAT(b,__SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)

#define __syscall(...) __SYSCALL_DISP(__syscall,__VA_ARGS__)

#endif /* !__LIBMINIX_RT_ASM_LIBMINIX_RT_H_ */
