#include "binfmt.h"

static int load_elf_binary(struct minix_rt_binprm *bprm)
{
    return -1;
}

static struct minix_rt_binfmt elf_format = {
    .load_binary = load_elf_binary,
};

int init_elf_binfmt(void)
{
	register_binfmt(&elf_format);
	return 0;
}

void exit_elf_binfmt(void)
{
	/* Remove the COFF and ELF loaders. */
	unregister_binfmt(&elf_format);
}
