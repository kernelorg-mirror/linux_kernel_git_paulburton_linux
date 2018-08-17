// SPDX-License-Identifier: GPL-2.0
#ifndef __ASM_UNROLL_H__
#define __ASM_UNROLL_H__

/*
 * Explicitly unroll a loop, for use in cases where doing so is performance
 * critical.
 *
 * Ideally we'd rely upon the compiler to provide this but there's no commonly
 * available means to do so. For example GCC's "#pragma GCC unroll"
 * functionality would be ideal but is only available from GCC 8 onwards. Using
 * -funroll-loops is an option but GCC tends to make poor choices when
 * compiling our string functions. -funroll-all-loops leads to massive code
 * bloat, even if only applied to the string functions.
 */
#define unroll(times, fn, ...) do {                             \
        extern void bad_unroll(void)                            \
                __compiletime_error("Unsupported unroll");      \
                                                                \
        /*                                                      \
         * We can't unroll if the number of iterations isn't    \
         * compile-time constant. Unfortunately GCC prior to    \
         * version 4.7 tends to miss obvious constants, so we   \
         * allow those versions to generate terrible but        \
         * functional code.                                     \
         */                                                     \
        BUILD_BUG_ON(GCC_VERSION >= 40700 &&                    \
                     !__builtin_constant_p(times));             \
                                                                \
        switch (times) {                                        \
        case 32: fn(__VA_ARGS__);                               \
        case 31: fn(__VA_ARGS__);                               \
        case 30: fn(__VA_ARGS__);                               \
        case 29: fn(__VA_ARGS__);                               \
        case 28: fn(__VA_ARGS__);                               \
        case 27: fn(__VA_ARGS__);                               \
        case 26: fn(__VA_ARGS__);                               \
        case 25: fn(__VA_ARGS__);                               \
        case 24: fn(__VA_ARGS__);                               \
        case 23: fn(__VA_ARGS__);                               \
        case 22: fn(__VA_ARGS__);                               \
        case 21: fn(__VA_ARGS__);                               \
        case 20: fn(__VA_ARGS__);                               \
        case 19: fn(__VA_ARGS__);                               \
        case 18: fn(__VA_ARGS__);                               \
        case 17: fn(__VA_ARGS__);                               \
        case 16: fn(__VA_ARGS__);                               \
        case 15: fn(__VA_ARGS__);                               \
        case 14: fn(__VA_ARGS__);                               \
        case 13: fn(__VA_ARGS__);                               \
        case 12: fn(__VA_ARGS__);                               \
        case 11: fn(__VA_ARGS__);                               \
        case 10: fn(__VA_ARGS__);                               \
        case 9: fn(__VA_ARGS__);                                \
        case 8: fn(__VA_ARGS__);                                \
        case 7: fn(__VA_ARGS__);                                \
        case 6: fn(__VA_ARGS__);                                \
        case 5: fn(__VA_ARGS__);                                \
        case 4: fn(__VA_ARGS__);                                \
        case 3: fn(__VA_ARGS__);                                \
        case 2: fn(__VA_ARGS__);                                \
        case 1: fn(__VA_ARGS__);                                \
        case 0: break;                                          \
                                                                \
        default:                                                \
                /*                                              \
                 * Either the iteration count is unreasonable   \
                 * or we need to add more cases above.          \
                 */                                             \
                bad_unroll();                                   \
                break;                                          \
        }                                                       \
} while (0)

#endif /* __ASM_UNROLL_H__ */
