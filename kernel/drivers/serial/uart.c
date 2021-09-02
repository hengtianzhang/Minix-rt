
#include <base/types.h>

#include <asm/fixmap.h>

struct pl01x_regs {
	u32	dr;		/* 0x00 Data register */
	u32	ecr;		/* 0x04 Error clear register (Write) */
	u32	pl010_lcrh;	/* 0x08 Line control register, high byte */
	u32	pl010_lcrm;	/* 0x0C Line control register, middle byte */
	u32	pl010_lcrl;	/* 0x10 Line control register, low byte */
	u32	pl010_cr;	/* 0x14 Control register */
	u32	fr;		/* 0x18 Flag register (Read only) */
#ifdef CONFIG_PL011_SERIAL_RLCR
	u32	pl011_rlcr;	/* 0x1c Receive line control register */
#else
	u32	reserved;
#endif
	u32	ilpr;		/* 0x20 IrDA low-power counter register */
	u32	pl011_ibrd;	/* 0x24 Integer baud rate register */
	u32	pl011_fbrd;	/* 0x28 Fractional baud rate register */
	u32	pl011_lcrh;	/* 0x2C Line control register */
	u32	pl011_cr;	/* 0x30 Control register */
};

#define UART_PL01x_RSR_OE               0x08
#define UART_PL01x_RSR_BE               0x04
#define UART_PL01x_RSR_PE               0x02
#define UART_PL01x_RSR_FE               0x01

#define UART_PL01x_FR_TXFE              0x80
#define UART_PL01x_FR_RXFF              0x40
#define UART_PL01x_FR_TXFF              0x20
#define UART_PL01x_FR_RXFE              0x10
#define UART_PL01x_FR_BUSY              0x08
#define UART_PL01x_FR_TMSK              (UART_PL01x_FR_TXFF + UART_PL01x_FR_BUSY)

/*
 *  PL010 definitions
 *
 */
#define UART_PL010_CR_LPE               (1 << 7)
#define UART_PL010_CR_RTIE              (1 << 6)
#define UART_PL010_CR_TIE               (1 << 5)
#define UART_PL010_CR_RIE               (1 << 4)
#define UART_PL010_CR_MSIE              (1 << 3)
#define UART_PL010_CR_IIRLP             (1 << 2)
#define UART_PL010_CR_SIREN             (1 << 1)
#define UART_PL010_CR_UARTEN            (1 << 0)

#define UART_PL010_LCRH_WLEN_8          (3 << 5)
#define UART_PL010_LCRH_WLEN_7          (2 << 5)
#define UART_PL010_LCRH_WLEN_6          (1 << 5)
#define UART_PL010_LCRH_WLEN_5          (0 << 5)
#define UART_PL010_LCRH_FEN             (1 << 4)
#define UART_PL010_LCRH_STP2            (1 << 3)
#define UART_PL010_LCRH_EPS             (1 << 2)
#define UART_PL010_LCRH_PEN             (1 << 1)
#define UART_PL010_LCRH_BRK             (1 << 0)


#define UART_PL010_BAUD_460800            1
#define UART_PL010_BAUD_230400            3
#define UART_PL010_BAUD_115200            7
#define UART_PL010_BAUD_57600             15
#define UART_PL010_BAUD_38400             23
#define UART_PL010_BAUD_19200             47
#define UART_PL010_BAUD_14400             63
#define UART_PL010_BAUD_9600              95
#define UART_PL010_BAUD_4800              191
#define UART_PL010_BAUD_2400              383
#define UART_PL010_BAUD_1200              767
/*
 *  PL011 definitions
 *
 */
#define UART_PL011_LCRH_SPS             (1 << 7)
#define UART_PL011_LCRH_WLEN_8          (3 << 5)
#define UART_PL011_LCRH_WLEN_7          (2 << 5)
#define UART_PL011_LCRH_WLEN_6          (1 << 5)
#define UART_PL011_LCRH_WLEN_5          (0 << 5)
#define UART_PL011_LCRH_FEN             (1 << 4)
#define UART_PL011_LCRH_STP2            (1 << 3)
#define UART_PL011_LCRH_EPS             (1 << 2)
#define UART_PL011_LCRH_PEN             (1 << 1)
#define UART_PL011_LCRH_BRK             (1 << 0)

#define UART_PL011_CR_CTSEN             (1 << 15)
#define UART_PL011_CR_RTSEN             (1 << 14)
#define UART_PL011_CR_OUT2              (1 << 13)
#define UART_PL011_CR_OUT1              (1 << 12)
#define UART_PL011_CR_RTS               (1 << 11)
#define UART_PL011_CR_DTR               (1 << 10)
#define UART_PL011_CR_RXE               (1 << 9)
#define UART_PL011_CR_TXE               (1 << 8)
#define UART_PL011_CR_LPE               (1 << 7)
#define UART_PL011_CR_IIRLP             (1 << 2)
#define UART_PL011_CR_SIREN             (1 << 1)
#define UART_PL011_CR_UARTEN            (1 << 0)

#define UART_PL011_IMSC_OEIM            (1 << 10)
#define UART_PL011_IMSC_BEIM            (1 << 9)
#define UART_PL011_IMSC_PEIM            (1 << 8)
#define UART_PL011_IMSC_FEIM            (1 << 7)
#define UART_PL011_IMSC_RTIM            (1 << 6)
#define UART_PL011_IMSC_TXIM            (1 << 5)
#define UART_PL011_IMSC_RXIM            (1 << 4)
#define UART_PL011_IMSC_DSRMIM          (1 << 3)
#define UART_PL011_IMSC_DCDMIM          (1 << 2)
#define UART_PL011_IMSC_CTSMIM          (1 << 1)
#define UART_PL011_IMSC_RIMIM           (1 << 0)

enum pl01x_type {
	TYPE_PL010,
	TYPE_PL011,
};

/* IO barriers */
#define __iormb(v)							\
({									\
	u64 tmp;						\
									\
	rmb();								\
									\
	/*								\
	 * Create a dummy control dependency from the IO read to any	\
	 * later instructions. This ensures that a subsequent call to	\
	 * udelay() will be ordered due to the ISB in get_cycles().	\
	 */								\
	asm volatile("eor	%0, %1, %1\n"				\
		     "cbnz	%0, ."					\
		     : "=r" (tmp) : "r" ((u64)(v))		\
		     : "memory");					\
})

#define dsb(opt)	asm volatile("dsb " #opt : : : "memory")

#define wmb()		dsb(st)

#define __iowmb()		wmb()

#define __raw_readl __raw_readl
static inline u32 __raw_readl(const volatile void  *addr)
{
	u32 val;
	asm volatile("ldr %w0, [%1]"
		     : "=r" (val) : "r" (addr));
	return val;
}

#define __raw_writel __raw_writel
static inline void __raw_writel(u32 val, volatile void  *addr)
{
	asm volatile("str %w0, [%1]" : : "rZ" (val), "r" (addr));
}

#define readl_relaxed(c)	({ u32 __r = (__le32)__raw_readl(c); __r; })
#define writel_relaxed(v,c)	((void)__raw_writel((u32)(v),(c)))
#define readl(c)		({ u32 __v = readl_relaxed(c); __iormb(__v); __v; })
#define writel(v,c)		({ __iowmb(); writel_relaxed((v),(c)); })

static struct pl01x_regs *base_regs = (struct pl01x_regs *)FIXADDR_START;

static int pl01x_putc(struct pl01x_regs *regs, char c)
{
	/* Wait until there is space in the FIFO */
	if (readl(&regs->fr) & UART_PL01x_FR_TXFF)
		return -1;

	/* Send the character */
	writel(c, &regs->dr);

	return 0;
}

static void pl01x_serial_putc(const char c)
{
	if (c == '\n')
		while (pl01x_putc(base_regs, '\r') == -1);

	while (pl01x_putc(base_regs, c) == -1);
}

void putc_q(const char c)
{
    pl01x_serial_putc(c);
}

void puts_q(const char *str)
{
	while (*str)
		putc_q(*str++);
}
