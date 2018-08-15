// SPDX-License-Identifier: GPL-2.0
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <asm/asm-eva.h>
#include <asm/cpu-features.h>
#include <asm/prefetch.h>
#include <asm/unroll.h>

#define protected_store(insn, fail_lbl, offset) do {	\
	asm_volatile_goto(				\
	"1:     " insn("$0", "%0") "\n"			\
	"       .section __ex_table,\"a\"\n"		\
	"       " __UA_ADDR "\t1b, %1\n"		\
	"       .previous"				\
	: /* no outputs */				\
	: "m"(dst[offset])				\
	: /* no clobbers */				\
	: fail_lbl);					\
} while (0)

unsigned long __clear_user(void __user *addr, unsigned long size)
{
	size_t line_size, prefetch_lines;
	char __user *dst = addr;
	int i;

	if (uaccess_kernel()) {
		memset((void *)addr, 0, size);
		return 0;
	}

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

	/* Prefetch a number of initial lines */
        i = 0;
	prefetch_lines = min_t(size_t, 128 / line_size, 2);
        unroll(prefetch_lines, pref_user_store_streamed, dst + (i++ * line_size));

	/* Store bytes until we're suitably aligned to store longs */
	while (size && unlikely((unsigned long)dst % sizeof(long))) {
		protected_store(user_sb, fault, 0);
		dst++;
		size--;
	}

	while (size >= line_size) {
		/* Prefetch the next line */
		pref_user_store_streamed(dst + (prefetch_lines * line_size));

		/* Fill the current line with zeroes */
                i = 0;
                unroll(line_size / sizeof(long),
                       protected_store, user_sd, fault, i++ * sizeof(long));

		/* Then move on */
		dst += line_size;
		size -= line_size;
	}

	/* Use as many long-sized stores as we can */
	while (size >= sizeof(long)) {
		protected_store(user_sd, fault, 0);
		dst += sizeof(long);
		size -= sizeof(long);
	}

	/* Use byte-sized stores for any remainder */
	while (unlikely(size)) {
		protected_store(user_sb, fault, 0);
		dst++;
		size--;
	}

	return 0;

fault:
	size -= current->thread.cp0_baduaddr - (unsigned long)dst;
	return size;
}
EXPORT_SYMBOL(__clear_user);
