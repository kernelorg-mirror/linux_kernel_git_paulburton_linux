/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2003 by Ralf Baechle
 */
#ifndef __ASM_PREFETCH_H
#define __ASM_PREFETCH_H

#include <asm/asm-eva.h>
#include <asm/cpu-info.h>

/*
 * R5000 and RM5200 implements pref and prefx instructions but they're nops, so
 * rather than wasting time we pretend these processors don't support
 * prefetching at all.
 *
 * R5432 implements Load, Store, LoadStreamed, StoreStreamed, LoadRetained,
 * StoreRetained and WriteBackInvalidate but not Pref_PrepareForStore.
 *
 * Hell (and the book on my shelf I can't open ...) know what the R8000 does.
 *
 * RM7000 version 1.0 interprets all hints as Pref_Load; version 2.0 implements
 * Pref_PrepareForStore also.
 *
 * RM9000 is MIPS IV but implements prefetching like MIPS32/MIPS64; it's
 * Pref_WriteBackInvalidate is a nop and Pref_PrepareForStore is broken in
 * current versions due to erratum G105.
 *
 * VR5500 (including VR5701 and VR7701) only implement load prefetch.
 *
 * Finally MIPS32 and MIPS64 implement all of the following hints.
 */

#define Pref_Load			0
#define Pref_Store			1
						/* 2 and 3 are reserved */
#define Pref_LoadStreamed		4
#define Pref_StoreStreamed		5
#define Pref_LoadRetained		6
#define Pref_StoreRetained		7
						/* 8 ... 24 are reserved */
#define Pref_WriteBackInvalidate	25
#define Pref_PrepareForStore		30

#define __GEN_PREF(mode, name, hint)					\
static inline void pref_##mode##_##name(const void __##mode *addr)	\
{									\
	/*								\
	 * If we aren't sure whether the CPU supports prefetch, don't   \
	 * emit them - the extra runtime checks are likely to cost us   \
	 * more than the prefetching will help.				\
	 */								\
	if (!__builtin_constant_p(cpu_has_prefetch))			\
		return;							\
									\
	if (cpu_has_prefetch)						\
		asm volatile(mode##_pref(hint, "%0")			\
			     : /* no outputs */				\
			     : "m"(*(const char *)addr));		\
}

__GEN_PREF(kernel, load, Pref_Load)
__GEN_PREF(kernel, store, Pref_Store)
__GEN_PREF(kernel, load_streamed, Pref_LoadStreamed)
__GEN_PREF(kernel, store_streamed, Pref_StoreStreamed)
__GEN_PREF(kernel, load_retained, Pref_LoadRetained)
__GEN_PREF(kernel, store_retained, Pref_StoreRetained)
__GEN_PREF(kernel, writeback_inv, Pref_WriteBackInvalidate)
__GEN_PREF(kernel, prepare_for_store, Pref_PrepareForStore)

__GEN_PREF(user, load, Pref_Load)
__GEN_PREF(user, store, Pref_Store)
__GEN_PREF(user, load_streamed, Pref_LoadStreamed)
__GEN_PREF(user, store_streamed, Pref_StoreStreamed)
__GEN_PREF(user, load_retained, Pref_LoadRetained)
__GEN_PREF(user, store_retained, Pref_StoreRetained)
__GEN_PREF(user, writeback_inv, Pref_WriteBackInvalidate)
__GEN_PREF(user, prepare_for_store, Pref_PrepareForStore)

#undef __GEN_PREF

#endif /* __ASM_PREFETCH_H */
