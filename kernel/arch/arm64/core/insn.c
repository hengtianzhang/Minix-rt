#include <base/sizes.h>
#include <base/errno.h>
#include <base/bitmap.h>

#include <asm/insn.h>

#define ADR_IMM_HILOSPLIT	2
#define ADR_IMM_SIZE		SZ_2M
#define ADR_IMM_LOMASK		((1 << ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_HIMASK		((ADR_IMM_SIZE >> ADR_IMM_HILOSPLIT) - 1)
#define ADR_IMM_LOSHIFT		29
#define ADR_IMM_HISHIFT		5

static int aarch64_get_imm_shift_mask(enum aarch64_insn_imm_type type,
						u32 *maskp, int *shiftp)
{
	u32 mask;
	int shift;

	switch (type) {
	case AARCH64_INSN_IMM_26:
		mask = BIT(26) - 1;
		shift = 0;
		break;
	case AARCH64_INSN_IMM_19:
		mask = BIT(19) - 1;
		shift = 5;
		break;
	case AARCH64_INSN_IMM_16:
		mask = BIT(16) - 1;
		shift = 5;
		break;
	case AARCH64_INSN_IMM_14:
		mask = BIT(14) - 1;
		shift = 5;
		break;
	case AARCH64_INSN_IMM_12:
		mask = BIT(12) - 1;
		shift = 10;
		break;
	case AARCH64_INSN_IMM_9:
		mask = BIT(9) - 1;
		shift = 12;
		break;
	case AARCH64_INSN_IMM_7:
		mask = BIT(7) - 1;
		shift = 15;
		break;
	case AARCH64_INSN_IMM_6:
	case AARCH64_INSN_IMM_S:
		mask = BIT(6) - 1;
		shift = 10;
		break;
	case AARCH64_INSN_IMM_R:
		mask = BIT(6) - 1;
		shift = 16;
		break;
	case AARCH64_INSN_IMM_N:
		mask = 1;
		shift = 22;
		break;
	default:
		return -EINVAL;
	}

	*maskp = mask;
	*shiftp = shift;

	return 0;
}

u64 aarch64_insn_decode_immediate(enum aarch64_insn_imm_type type, u32 insn)
{
	u32 immlo, immhi, mask;
	int shift;

	switch (type) {
	case AARCH64_INSN_IMM_ADR:
		shift = 0;
		immlo = (insn >> ADR_IMM_LOSHIFT) & ADR_IMM_LOMASK;
		immhi = (insn >> ADR_IMM_HISHIFT) & ADR_IMM_HIMASK;
		insn = (immhi << ADR_IMM_HILOSPLIT) | immlo;
		mask = ADR_IMM_SIZE - 1;
		break;
	default:
		if (aarch64_get_imm_shift_mask(type, &mask, &shift) < 0) {
			printf("aarch64_insn_decode_immediate: unknown immediate encoding %d\n",
			       type);
			return 0;
		}
	}

	return (insn >> shift) & mask;
}

u32 aarch64_insn_decode_register(enum aarch64_insn_register_type type,
					u32 insn)
{
	int shift;

	switch (type) {
	case AARCH64_INSN_REGTYPE_RT:
	case AARCH64_INSN_REGTYPE_RD:
		shift = 0;
		break;
	case AARCH64_INSN_REGTYPE_RN:
		shift = 5;
		break;
	case AARCH64_INSN_REGTYPE_RT2:
	case AARCH64_INSN_REGTYPE_RA:
		shift = 10;
		break;
	case AARCH64_INSN_REGTYPE_RM:
		shift = 16;
		break;
	default:
		printf("%s: unknown register type encoding %d\n", __func__,
		       type);
		return 0;
	}

	return (insn >> shift) & GENMASK(4, 0);
}
