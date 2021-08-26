#ifndef __SEL4M_COMPILER_H_
#define __SEL4M_COMPILER_H_

/*
 * The attributes in this file are unconditionally defined and they directly
 * map to compiler attribute(s), unless one of the compilers does not support
 * the attribute. In that case, __has_attribute is used to check for support
 * and the reason is stated in its comment ("Optional: ...").
 *
 * Any other "attributes" (i.e. those that depend on a configuration option,
 * on a compiler, on an architecture, on plugins, on other attributes...)
 * should be defined elsewhere (e.g. compiler_types.h or compiler-*.h).
 * The intention is to keep this file as simple as possible, as well as
 * compiler- and version-agnostic (e.g. avoiding GCC_VERSION checks).
 *
 * This file is meant to be sorted (by actual attribute name,
 * not by #define identifier). Use the __attribute__((__name__)) syntax
 * (i.e. with underscores) to avoid future collisions with other macros.
 * Provide links to the documentation of each supported compiler, if it exists.
 */

/*
 * __has_attribute is supported on gcc >= 5, clang >= 2.9 and icc >= 17.
 * In the meantime, to support 4.6 <= gcc < 5, we implement __has_attribute
 * by hand.
 *
 * sparse does not support __has_attribute (yet) and defines __GNUC_MINOR__
 * depending on the compiler used to build it; however, these attributes have
 * no semantic effects for sparse, so it does not matter. Also note that,
 * in order to avoid sparse's warnings, even the unsupported ones must be
 * defined to 0.
 */
#ifndef __has_attribute
# define __has_attribute(x) __GCC4_has_attribute_##x
# define __GCC4_has_attribute___assume_aligned__      (__GNUC_MINOR__ >= 9)
# define __GCC4_has_attribute___copy__                0
# define __GCC4_has_attribute___designated_init__     0
# define __GCC4_has_attribute___externally_visible__  1
# define __GCC4_has_attribute___noclone__             1
# define __GCC4_has_attribute___nonstring__           0
# define __GCC4_has_attribute___no_sanitize_address__ (__GNUC_MINOR__ >= 8)
#endif

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

/*
 * Optional: not supported by clang
 *
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-externally_005fvisible-function-attribute
 */
#if __has_attribute(__externally_visible__)
# define __visible                      __attribute__((__externally_visible__))
#else
# define __visible
#endif

#endif /* !__SEL4M_COMPILER_H_ */
