// SPDX-License-Identifier: GPL-2.0
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/cpu-features.h>
#include <asm/isa-rev.h>
#include <asm/prefetch.h>
#include <asm/unroll.h>

static unsigned long splat_u8_long(unsigned char val)
{
	unsigned long l = val;

	if (MIPS_ISA_REV >= 2) {
		asm("ins\t%0, %0, 8, 8" : "+r"(l));
		asm("ins\t%0, %0, 16, 16" : "+r"(l));
#ifdef CONFIG_64BIT
		asm("dins\t%0, %0, 32, 32" : "+r"(l));
#endif
	} else {
		l |= l << 8;
		l |= l << 16;
#ifdef CONFIG_64BIT
		l |= l << 32;
#endif
	}

	return l;
}

void *memset(void *addr, int ch, size_t size)
{
	size_t line_size, prefetch_lines;
	unsigned long fill_long;
	char *dst = addr;
	int i;

	fill_long = splat_u8_long(ch);

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
		*dst++ = ch;
		size--;
	}

	while (size >= line_size) {
		/* Prefetch the next line */
		pref_kernel_store_streamed(dst + (prefetch_lines * line_size));

#define store(dst) do {				\
	*(unsigned long *)(dst) = fill_long;	\
} while (0)

		/* Fill the current line */
		i = 0;
		unroll(line_size / sizeof(long), store, dst + (i++ * sizeof(long)));

		/* Then move on */
		dst += line_size;
		size -= line_size;
	}

	/* Use as many long-sized stores as we can */
	while (size >= sizeof(long)) {
		*(unsigned long *)dst = fill_long;
		dst += sizeof(long);
		size -= sizeof(long);
	}

	/* Use byte-sized stores for any remainder */
	while (unlikely(size)) {
		*dst++ = ch;
		size--;
	}

	return addr;
}
EXPORT_SYMBOL(memset);
