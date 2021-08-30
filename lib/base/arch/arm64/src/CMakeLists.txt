
target_sources(
	${base_VAR}base
	PRIVATE
	memchr.S memcmp.S memcpy.S memmove.S memset.S
	strchr.S strcmp.S strlen.S strncmp.S strnlen.S
	strrchr.S
)
