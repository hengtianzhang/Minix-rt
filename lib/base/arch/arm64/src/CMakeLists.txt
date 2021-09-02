
target_sources(
	${base_VAR}base
	PRIVATE
	memchr.S memcmp.S memcpy.S memmove.S memset.S
	strchr.S strcmp.S strlen.S strncmp.S strnlen.S
	strrchr.S
)

target_sources_ifdef(CONFIG_CRC32
	${base_VAR}base
	PRIVATE
	crc32.S
)
