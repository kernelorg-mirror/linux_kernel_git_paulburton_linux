// SPDX-License-Identifier: GPL-2.0
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <asm/asm-eva.h>
#include <asm/cpu-features.h>
#include <asm/prefetch.h>
#include <asm/unroll.h>

/* Generate the standard in-kernel memcpy() */
#define __func memcpy
#define __src kernel
#define __dst kernel
#define __ret dst
#define __ret_type void *
#include "memcpy.h"

/* Now generate an in-user copy function */
#define __func mips_copy_user
#define __src user
#define __dst user
#define __ret size
#define __ret_type size_t
#include "memcpy.h"

#ifdef CONFIG_EVA

/*
 * For EVA systems we can't use mips_copy_user() when copying between user &
 * kernel memory, because each requires different memory access instructions.
 *
 * We generate dedicated copy routines for kernel-to-user & user-to-kernel
 * copies here, which will use the appropriate EVA or non-EVA load & store
 * instructions.
 */

# define __func mips_copy_from_user
# define __src user
# define __dst kernel
# define __ret size
# define __ret_type size_t
# include "memcpy.h"

# define __func mips_copy_to_user
# define __src kernel
# define __dst user
# define __ret size
# define __ret_type size_t
# include "memcpy.h"

#else /* !CONFIG_EVA */

/*
 * For non-EVA systems we can use mips_copy_user() even when copying between
 * user & kernel memory. Since both use the same standard load & store
 * instructions the only difference this has with dedicated functions for each
 * type of copy is that we have __ex_table entries for kernel the memory
 * accesses. Since those should never generate an exception that should be
 * harmless.
 *
 * We declare mips_copy_from_user() & mips_copy_to_user() as aliases to
 * mips_copy_user() such that users of these functions in asm/uaccess.h don't
 * need to care about EVA.
 */

size_t mips_copy_from_user(void *dst, const void *src, size_t size)
	__alias(mips_copy_user);

size_t mips_copy_to_user(void *dst, const void *src, size_t size)
	__alias(mips_copy_user);

EXPORT_SYMBOL(mips_copy_from_user);
EXPORT_SYMBOL(mips_copy_to_user);

#endif /* !CONFIG_EVA */

void *memmove(void *dst, const void *src, size_t size)
{
	const char *s;
	char *d;

	/* Use our optimized memcpy() when appropriate */
	if ((dst + size <= src) || (dst >= src + size))
		return memcpy(dst, src, size);

	/* Fall back to copying in reverse */
	for (d = dst + size, s = src + size; size; size--)
		*--d = *--s;

	return dst;
}
EXPORT_SYMBOL(memmove);
