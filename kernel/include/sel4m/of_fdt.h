#ifndef __SEL4M_OF_FDT_H_
#define __SEL4M_OF_FDT_H_

#include <of/of_fdt.h>

#define COMMAND_LINE_SIZE 2048

extern char boot_command_line[COMMAND_LINE_SIZE];

extern phys_addr_t phys_initrd_start;
extern phys_addr_t phys_initrd_size;

#endif /* !__SEL4M_OF_FDT_H_ */
