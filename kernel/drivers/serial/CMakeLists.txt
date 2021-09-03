
kernel_library()

kernel_library_sources(uart.c)

kernel_library_sources_ifdef(
	CONFIG_SERIAL_AMBA_PL011
	uart_pl011.c
)
