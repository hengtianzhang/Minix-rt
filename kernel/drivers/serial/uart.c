/*
 *(C) Copyright Black Sesame Technologies (Shanghai)Ltd. Co., 2020. All rights reserved.
 *
 * This file contains proprietary information that is the sole intellectual property of Black
 * Sesame Technologies (Shanghai)Ltd. Co. No part of this material or its documentation 
 * may be reproduced, distributed, transmitted, displayed or published in any manner 
 * without the written permission of Black Sesame Technologies (Shanghai)Ltd. Co.
 */

#include <base/common.h>
#include <base/init.h>
#include <base/errno.h>

#include <sel4m/console.h>
#include <sel4m/of_fdt.h>

#include <asm/fixmap.h>

#include "uart.h"

static int uart_init_done = 0;
extern struct tty_uart_platdata plat;

struct puts_queue {
	int head;
	int tail;
	int max;
	char buf[8192];
};

static struct puts_queue putsq = {
	.head = 0,
	.tail = 0,
	.max = 8192,
	.buf = {'\0'},
};

static void puts_q_write(const char *str)
{
	while (*str) {
		putsq.buf[putsq.head % putsq.max] = *str;
		putsq.head++;
		str++;
	}

	putsq.buf[putsq.head % putsq.max] = *str;
}

static int pus_q_is_empty(void)
{
	return putsq.head == putsq.tail;
}

static char *puts_q_read(void)
{
	char *buf;

	buf = &putsq.buf[putsq.tail];
	putsq.tail = putsq.head;

	return buf;
}

void uart_init(int port, int baudrate)
{
    plat.init(&plat, port, baudrate);
}

struct uart_port {
	phys_addr_t mapbase;
	phys_addr_t mapsize;
};

struct uart_port port;

static void flush_uart(void)
{
	printf("\n");
}

void early_uart(struct device_node *node)
{
	uart_init(0, 115200);

	uart_init_done = 1;

	flush_uart();
}

CONSOLE_DECLARE(pl011, "arm,pl011", early_uart);

void *uart_virt_base;

typedef void (*console_init_cb_t)(struct device_node *node);

int __init of_setup_console(const struct of_device_id *match,
			     u64 node,
			     const char *options)
{
	u64 addr;
	console_init_cb_t cb;

	printf("console match %s", match->compatible);

	addr = of_flat_dt_translate_address(node);
	if (addr == OF_BAD_ADDR) {
		printf("[%s] bad address\n", match->name);
		return -ENXIO;
	}
	port.mapbase = addr;

	uart_virt_base = __fixmap_remap_console(port.mapbase, FIXMAP_PAGE_IO);

	cb = match->data;

	cb(NULL);

	return 0;
}

int uart_setbrg(int port, int baudrate)
{
	return plat.setbrg(&plat, port, baudrate);
}

int uart_pending(bool input)
{
    return plat.pending(&plat, input);
}

int getc(void)
{
    return plat.getc(&plat);
}

int tstc(void)
{
    return plat.pending(&plat, true);
}

void putc_q(const char c)
{
    plat.putc(&plat, c);
}

void puts_q(const char *str)
{
	if (!uart_init_done) {
		puts_q_write(str);
		return;
	}

	if (!pus_q_is_empty()) {
		char *buf;

		buf = puts_q_read();

		while (*buf)
			putc_q(*buf++);
		
		return;
	}

	while (*str)
		putc_q(*str++);
}

int uart_get_baudrate(void)
{
    return plat.baudrate;
}

int uart_get_port(void)
{
    return plat.port;
}
