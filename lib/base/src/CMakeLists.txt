
target_sources(
	${base_VAR}base
	PRIVATE
	string.c ctype.c hexdump.c find_bit.c bitmap.c
	div64.c vsprintf.c kstrtox.c crc32.c bsearch.c
	sort.c list_sort.c random.c time.c timeconv.c rbtree.c
)

target_sources_ifdef(CONFIG_GENERIC_HWEIGHT
	${base_VAR}base
	PRIVATE
	hweight.c
)

target_sources_ifdef(CONFIG_BITREVERSE
	${base_VAR}base
	PRIVATE
	bitrev.c
)
