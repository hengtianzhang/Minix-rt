#ifndef __KERNEL_VERSION_H_
#define __KERNEL_VERSION_H_

/*  values come from cmake/version.cmake */

#define SEL4M_VERSION_CODE 257
#define SEL4M_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#define KERNELVERSION          0x10100
#define KERNEL_VERSION_NUMBER  0x101
#define KERNEL_VERSION_MAJOR   0
#define KERNEL_VERSION_MINOR   1
#define KERNEL_PATCHLEVEL      1
#define KERNEL_VERSION_STRING  "0.1.1"

#endif /* !__KERNEL_VERSION_H_ */
