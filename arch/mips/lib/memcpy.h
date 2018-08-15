// SPDX-License-Identifier: GPL-2.0

#define ex_table_kernel(addr, fixup)

#define ex_table_user(addr, fixup)					\
	"       .section __ex_table,\"a\"\n"				\
	"       " __UA_ADDR "\t" addr ", " fixup "\n"			\
	"       .previous"

#define _protected_copy(insn, dst_mode, dst, src_mode, src) do {	\
	asm_volatile_goto(						\
	"       .set push\n"						\
	"       .set noat\n"						\
	"1:     " src_mode##_l##insn("$1", "%0") "\n"			\
	"2:     " dst_mode##_s##insn("$1", "%1") "\n"			\
	"       .set pop\n"						\
	ex_table_##src_mode("1b", "%2")					\
	ex_table_##dst_mode("2b", "%3")					\
	: /* no outputs */						\
	: "m"(src), "m"(dst)						\
	: "memory"							\
	: fault_src, fault_dst);					\
} while (0)

#define protected_copy(insn, dst_mode, dst, src_mode, src)		\
	_protected_copy(insn, dst_mode, dst, src_mode, src)

#define _protected_copy4(insn, dst_mode, dst, src_mode, src, post) do {	\
        const unsigned long *_s = (src);                                \
        unsigned long *_d = (dst);                                      \
                                                                        \
	asm_volatile_goto(						\
	"1:     " src_mode##_l##insn("$8", "%0") "\n"			\
	"2:     " src_mode##_l##insn("$9", "%1") "\n"			\
	"3:     " src_mode##_l##insn("$10", "%2") "\n"			\
	"4:     " src_mode##_l##insn("$11", "%3") "\n"			\
	"5:     " dst_mode##_s##insn("$8", "%4") "\n"			\
	"6:     " dst_mode##_s##insn("$9", "%5") "\n"			\
	"7:     " dst_mode##_s##insn("$10", "%6") "\n"			\
	"8:     " dst_mode##_s##insn("$11", "%7") "\n"			\
	ex_table_##src_mode("1b", "%8")					\
	ex_table_##src_mode("2b", "%8")					\
	ex_table_##src_mode("3b", "%8")					\
	ex_table_##src_mode("4b", "%8")					\
	ex_table_##dst_mode("5b", "%9")					\
	ex_table_##dst_mode("6b", "%9")					\
	ex_table_##dst_mode("7b", "%9")					\
	ex_table_##dst_mode("8b", "%9")					\
	: /* no outputs */						\
	: "m"(_s[0]), "m"(_s[1]), "m"(_s[2]), "m"(_s[3]),		\
	  "m"(_d[0]), "m"(_d[1]), "m"(_d[2]), "m"(_d[3])		\
	: "$8", "$9", "$10", "$11", "memory"				\
	: fault_src, fault_dst);					\
                                                                        \
        post;                                                           \
} while (0)

#define protected_copy4(insn, dst_mode, dst, src_mode, src, post)	\
	_protected_copy4(insn, dst_mode, dst, src_mode, src, post)

#define _pref_both(__src, __dst, offset) do {   \
        pref_##__src##_load(src + (offset));    \
        pref_##__dst##_store(dst + (offset));   \
} while (0)

#define pref_both(__src, __dst, offset)         \
        _pref_both(__src, __dst, offset)

__ret_type __func(void *dst, const void *src, size_t size)
{
	size_t line_size, prefetch_lines;
	int i;
	union {
		char *ch;
		unsigned long *ul;
		unsigned long addr;
	} d = { dst };
	union {
		const char *ch;
		unsigned long *ul;
		unsigned long addr;
	} s = { src };

	/*
	 * If the source & destination aren't similarly aligned with regards to
	 * the size of a long then fall back to a straightforward but slow byte
	 * copy.
	 */
	if (unlikely((d.addr % sizeof(long)) != (s.addr % sizeof(long))))
		goto remaining_bytes;

	/*
	 * If we know the dcache line size for sure then we use that here,
	 * otherwise we choose a conservative yet common default to optimize
	 * for. Having line_size be too small isn't a big deal - we'll just
	 * emit extra pref instructions that will effectively be nops.
	 */
	if (__builtin_constant_p(cpu_dcache_line_size()))
		line_size = cpu_dcache_line_size();
	else
		line_size = 32;

	/* ...and how many lines to prefetch */
	prefetch_lines = min_t(size_t, 128 / line_size, 2);

	/* Prefetch the first lines */
        i = 0;
        unroll(prefetch_lines, pref_both, __src, __dst, i++ * line_size);

	/* Copy bytes until we're suitably aligned to copy longs */
	while (size && unlikely(s.addr % sizeof(long))) {
		protected_copy(b, __dst, d.ch[0], __src, s.ch[0]);
		d.addr++;
		s.addr++;
		size--;
	}

	while (size >= line_size) {
		/* Prefetch the next line */
                pref_both(__src, __dst, prefetch_lines * line_size);

		/* Copy the current line */
                i = 0;
                unroll(line_size / (sizeof(long) * 4), protected_copy4, d,
                       __dst, &d.ul[i * 4],
                       __src, &s.ul[i * 4],
                       i++);

		/* Then move on */
		d.addr += line_size;
		s.addr += line_size;
		size -= line_size;
	}

	/* Copy as many longs as we can */
	while (size >= sizeof(long)) {
		protected_copy(d, __dst, d.ul[0], __src, s.ul[0]);
		d.addr += sizeof(long);
		s.addr += sizeof(long);
		size -= sizeof(long);
	}

remaining_bytes:
        /* Copy any remaining bytes */
	while (size) {
		protected_copy(b, __dst, d.ch[0], __src, s.ch[0]);
		d.addr++;
		s.addr++;
		size--;
	}

	return __ret;

fault_src:
	size -= current->thread.cp0_baduaddr - (unsigned long)src;
	return __ret;

fault_dst:
	size -= current->thread.cp0_baduaddr - (unsigned long)dst;
	return __ret;
}
EXPORT_SYMBOL(__func);

#undef __func
#undef __src
#undef __dst
#undef __ret
#undef __ret_type
