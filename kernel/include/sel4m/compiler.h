#ifndef __SEL4M_COMPILER_H_
#define __SEL4M_COMPILER_H_

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-cold-function-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html#index-cold-label-attribute
 */
#define __cold                          __attribute__((__cold__))

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-section-function-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-section-variable-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#section-declspec-allocate
 */
#define __section(S)                    __attribute__((__section__(#S)))

#endif /* !__SEL4M_COMPILER_H_ */
