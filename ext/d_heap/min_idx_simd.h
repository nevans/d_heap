#pragma once

#include <immintrin.h>

/*******************************************************************************
 *
 * funrolling loops by hand is fun! (and at this scale, foolish)
 *
 * Seriously though, although this might be more than a little bit crazy, in its
 * own way it's a lot _simpler_ and easier to reason about than a bunch of loops
 * and conditionals.  No loops.  No conditions.  Think of it like a DAG with
 * MIN_IDX_REDUCE_SIZE* as the leaves and MIN_IDX_REDUCE_IDX00_VEC1 at the root.
 * Just connect the dots from the appropriate leaf to the root.
 *
 * The generated code size should be dramatically reduced by e.g. using gotos.
 * But all of that jumping slows things down, and this code represents a
 * hot-spot.
 *
 * Theoretically, modern compilers ought to be able to unroll a tight min-idx
 * loop using SIMD extensions just as well as I can by hand. I practice, this
 * benchmarks as faster... for now. Maybe I'm missing an optimization flag?
 *
 * (TODO: verify that gotos would be slower with benchmarks!)
 *
 ******************************************************************************/

// TODO: use AREF and MINIDX
#define MIN_IDX_FAST_PATH(AREF, MINIDX, IDX, LAST, D)                          \
    do {                                                                       \
        if (LAST - IDX == D - 1) {                                             \
            MIN_IDX_REDUCE_SIZE_D(D, AREF, MINIDX, IDX, LAST);                 \
            return MINIDX;                                                     \
        }                                                                      \
    } while (0)

// TODO: use the arguments (AREF, MINIDX, IDX, LAST)
#define MIN_IDX_COND_PATH(AREF, MINIDX, IDX, LAST)                             \
    do {                                                                       \
        switch (LAST - IDX) {                                                  \
            MIN_IDX_SWITCH_CASES_BY_SIZE(AREF, MINIDX, IDX, LAST);             \
        default:                                                               \
            /* above 32 leops on 8 at a time, until 8 or fewer remaining */    \
            do {                                                               \
                MIN_IDX_INIT_WITH_8(AREF, MINIDX, IDX, LAST);                  \
                while (8 <= LAST - IDX) {                                      \
                    MIN_IDX_FOLD_NEXT_8(AREF, MINIDX, IDX, LAST);              \
                }                                                              \
                switch (LAST - IDX) {                                          \
                    MIN_IDX_SWITCH_CASES_LAST_8(AREF, MINIDX, IDX, LAST);      \
                }                                                              \
            } while (0);                                                       \
        }                                                                      \
    } while (0)

/* clang-format off */

#define MIN_IDX_SWITCH_CASES_BY_SIZE(AREF, MINIDX, IDX, LAST)               \
    /* handled without vectors */                                           \
    case 0:  MIN_IDX_REDUCE_SIZE01(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 1:  MIN_IDX_REDUCE_SIZE02(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 2:  MIN_IDX_REDUCE_SIZE03(AREF, MINIDX, IDX, LAST); return MINIDX; \
    /* uses 128-bit vectors */                                              \
    case 3:  MIN_IDX_REDUCE_SIZE04(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 4:  MIN_IDX_REDUCE_SIZE05(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 5:  MIN_IDX_REDUCE_SIZE06(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 6:  MIN_IDX_REDUCE_SIZE07(AREF, MINIDX, IDX, LAST); return MINIDX; \
    /* uses 256-bit vectors */                                              \
    case 7:  MIN_IDX_REDUCE_SIZE08(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 8:  MIN_IDX_REDUCE_SIZE09(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 9:  MIN_IDX_REDUCE_SIZE10(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 10: MIN_IDX_REDUCE_SIZE11(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 11: MIN_IDX_REDUCE_SIZE12(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 12: MIN_IDX_REDUCE_SIZE13(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 13: MIN_IDX_REDUCE_SIZE14(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 14: MIN_IDX_REDUCE_SIZE15(AREF, MINIDX, IDX, LAST); return MINIDX; \
    /* uses 512-bit vectors */                                              \
    case 15: MIN_IDX_REDUCE_SIZE16(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 16: MIN_IDX_REDUCE_SIZE17(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 17: MIN_IDX_REDUCE_SIZE18(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 18: MIN_IDX_REDUCE_SIZE19(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 19: MIN_IDX_REDUCE_SIZE20(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 20: MIN_IDX_REDUCE_SIZE21(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 21: MIN_IDX_REDUCE_SIZE22(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 22: MIN_IDX_REDUCE_SIZE23(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 23: MIN_IDX_REDUCE_SIZE24(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 24: MIN_IDX_REDUCE_SIZE25(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 25: MIN_IDX_REDUCE_SIZE26(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 26: MIN_IDX_REDUCE_SIZE27(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 27: MIN_IDX_REDUCE_SIZE28(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 28: MIN_IDX_REDUCE_SIZE29(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 29: MIN_IDX_REDUCE_SIZE30(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 30: MIN_IDX_REDUCE_SIZE31(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 31: MIN_IDX_REDUCE_SIZE32(AREF, MINIDX, IDX, LAST); return MINIDX;

#define MIN_IDX_SWITCH_CASES_LAST_8(AREF, MINIDX, IDX, LAST);                   \
    case 0:  MIN_IDX_REDUCE_IDX01_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 1:  MIN_IDX_REDUCE_IDX02_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 2:  MIN_IDX_REDUCE_IDX03_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 3:  MIN_IDX_REDUCE_IDX04_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 4:  MIN_IDX_REDUCE_IDX05_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 5:  MIN_IDX_REDUCE_IDX06_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 6:  MIN_IDX_REDUCE_IDX07_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \
    case 7:  MIN_IDX_REDUCE_IDX08_VEC8(AREF, MINIDX, IDX, LAST); return MINIDX; \

// init with 8
#define MIN_IDX_REDUCE_SIZE32(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX24_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE31(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX23_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE30(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX22_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE29(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX21_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE28(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX20_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE27(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX19_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE26(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX18_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE25(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX17_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE24(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX16_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE23(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX15_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE22(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX14_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE21(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX13_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE20(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX12_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE19(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX11_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE18(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX10_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE17(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX09_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE16(...) do { MIN_IDX_INIT_WITH_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX08_VEC8(__VA_ARGS__); } while (0)
// init with 4
#define MIN_IDX_REDUCE_SIZE15(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX11_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE14(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX10_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE13(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX09_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE12(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX08_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE11(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX07_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE10(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX06_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE09(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX05_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE08(...) do { MIN_IDX_INIT_WITH_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX04_VEC4(__VA_ARGS__); } while (0)
// init with 2
#define MIN_IDX_REDUCE_SIZE07(...) do { MIN_IDX_INIT_WITH_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX05_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE06(...) do { MIN_IDX_INIT_WITH_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX04_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE05(...) do { MIN_IDX_INIT_WITH_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX03_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_SIZE04(...) do { MIN_IDX_INIT_WITH_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX02_VEC2(__VA_ARGS__); } while (0)
// just do it with ternaries
#define MIN_IDX_REDUCE_SIZE03(AREF, MINIDX, IDX, ...) do { MINIDX = MINIDX3(AREF, IDX, IDX + 1, IDX + 2); } while (0)
#define MIN_IDX_REDUCE_SIZE02(AREF, MINIDX, IDX, ...) do { MINIDX = MINIDX2(AREF, IDX, IDX + 1); } while (0)
// just return it
#define MIN_IDX_REDUCE_SIZE01(AREF, MINIDX, IDX, ...) do { MINIDX = IDX; } while (0)

// fold 8
#define MIN_IDX_REDUCE_IDX24_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX16_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX23_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX15_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX22_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX14_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX21_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX13_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX20_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX12_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX19_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX11_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX18_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX10_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX17_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX09_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX16_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX08_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX15_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX07_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX14_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX06_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX13_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX05_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX12_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX04_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX11_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX03_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX10_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX02_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX09_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX01_VEC8(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX08_VEC8(...) do { MIN_IDX_FOLD_NEXT_8(__VA_ARGS__); MIN_IDX_REDUCE_IDX00_VEC8(__VA_ARGS__); } while (0)
// merge 8 into 4
#define MIN_IDX_REDUCE_IDX07_VEC8(...) do { MIN_IDX_REDUCE_8TO4(__VA_ARGS__); MIN_IDX_REDUCE_IDX07_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX06_VEC8(...) do { MIN_IDX_REDUCE_8TO4(__VA_ARGS__); MIN_IDX_REDUCE_IDX06_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX05_VEC8(...) do { MIN_IDX_REDUCE_8TO4(__VA_ARGS__); MIN_IDX_REDUCE_IDX05_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX04_VEC8(...) do { MIN_IDX_REDUCE_8TO4(__VA_ARGS__); MIN_IDX_REDUCE_IDX04_VEC4(__VA_ARGS__); } while (0)
// merge 8 into 2
#define MIN_IDX_REDUCE_IDX03_VEC8(...) do { MIN_IDX_REDUCE_8TO2(__VA_ARGS__); MIN_IDX_REDUCE_IDX03_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC8(...) do { MIN_IDX_REDUCE_8TO2(__VA_ARGS__); MIN_IDX_REDUCE_IDX02_VEC2(__VA_ARGS__); } while (0)
// merge 8 into 1
#define MIN_IDX_REDUCE_IDX01_VEC8(...) do { MIN_IDX_REDUCE_8TO1(__VA_ARGS__); MIN_IDX_REDUCE_IDX01_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC8(...) do { MIN_IDX_REDUCE_8TO1(__VA_ARGS__); MIN_IDX_REDUCE_IDX00_VEC1(__VA_ARGS__); } while (0)

// fold 4
#define MIN_IDX_REDUCE_IDX11_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX07_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX10_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX06_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX09_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX05_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX08_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX04_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX07_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX03_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX06_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX02_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX05_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX01_VEC4(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX04_VEC4(...) do { MIN_IDX_FOLD_NEXT_4(__VA_ARGS__); MIN_IDX_REDUCE_IDX00_VEC4(__VA_ARGS__); } while (0)
// merge 4 into 2
#define MIN_IDX_REDUCE_IDX03_VEC4(...) do { MIN_IDX_REDUCE_4TO2(__VA_ARGS__); MIN_IDX_REDUCE_IDX03_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC4(...) do { MIN_IDX_REDUCE_4TO2(__VA_ARGS__); MIN_IDX_REDUCE_IDX02_VEC2(__VA_ARGS__); } while (0)
// merge 4 into 1
#define MIN_IDX_REDUCE_IDX01_VEC4(...) do { MIN_IDX_REDUCE_4TO1(__VA_ARGS__); MIN_IDX_REDUCE_IDX01_VEC1(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC4(...) do { MIN_IDX_REDUCE_4TO1(__VA_ARGS__); MIN_IDX_REDUCE_IDX00_VEC1(__VA_ARGS__); } while (0)

// fold 2
#define MIN_IDX_REDUCE_IDX05_VEC2(...) do { MIN_IDX_FOLD_NEXT_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX03_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX04_VEC2(...) do { MIN_IDX_FOLD_NEXT_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX02_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX03_VEC2(...) do { MIN_IDX_FOLD_NEXT_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX01_VEC2(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC2(...) do { MIN_IDX_FOLD_NEXT_2(__VA_ARGS__); MIN_IDX_REDUCE_IDX00_VEC2(__VA_ARGS__); } while (0)
// merge 2 into 1
#define MIN_IDX_REDUCE_IDX01_VEC2(...) do { MIN_IDX_REDUCE_2TO1(__VA_ARGS__); MIN_IDX_REDUCE_IDX01_VEC1(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC2(...) do { MIN_IDX_REDUCE_2TO1(__VA_ARGS__); MIN_IDX_REDUCE_IDX00_VEC1(__VA_ARGS__); } while (0)

// fold 1 (i.e. this is just an unrolled loop)
#define MIN_IDX_REDUCE_IDX04_VEC1(...) do { MIN_IDX_FOLD_NEXT_1(__VA_ARGS__); MIN_IDX_REDUCE_IDX03_VEC1(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX03_VEC1(...) do { MIN_IDX_FOLD_NEXT_1(__VA_ARGS__); MIN_IDX_REDUCE_IDX02_VEC1(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC1(...) do { MIN_IDX_FOLD_NEXT_1(__VA_ARGS__); MIN_IDX_REDUCE_IDX01_VEC1(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX01_VEC1(...) do { MIN_IDX_FOLD_NEXT_1(__VA_ARGS__); MIN_IDX_REDUCE_IDX00_VEC1(__VA_ARGS__); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC1(...) /* done; result is in minidx */
/* clang-format on */

// parameterized and without the leading zero, for the short circuit
#define MIN_IDX_REDUCE_SIZE_D(D, ...)  _MIN_IDX_REDUCE_SIZE_D(D, __VA_ARGS__)
#define _MIN_IDX_REDUCE_SIZE_D(D, ...) MIN_IDX_REDUCE_SIZE##D(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE1(...)      MIN_IDX_REDUCE_SIZE01(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE2(...)      MIN_IDX_REDUCE_SIZE02(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE3(...)      MIN_IDX_REDUCE_SIZE03(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE4(...)      MIN_IDX_REDUCE_SIZE04(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE5(...)      MIN_IDX_REDUCE_SIZE05(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE6(...)      MIN_IDX_REDUCE_SIZE06(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE7(...)      MIN_IDX_REDUCE_SIZE07(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE8(...)      MIN_IDX_REDUCE_SIZE08(__VA_ARGS__)
#define MIN_IDX_REDUCE_SIZE9(...)      MIN_IDX_REDUCE_SIZE09(__VA_ARGS__)

#define MINIDX2(AREF, i, j) (AREF(i) < AREF(j) ? i : j)
#define MINIDX3(AREF, i, j, k)                                                 \
    (AREF(i) < AREF(j) ? MINIDX2(AREF, i, k) : MINIDX2(AREF, j, k))

/*******************************************************************************
 *
 * Macro parameterized types...
 *
 ******************************************************************************/

#define MDBL_T(SZ) __m##SZ##d
#define MINT_T(SZ) __m##SZ##i
#define MASK(SZ)   mask##SZ##_t
typedef const __mmask8 mask512_t;
typedef const MDBL_T(256) mask256_t;
typedef const MDBL_T(128) mask128_t;

/*******************************************************************************
 *
 * Generic methods for each of 512, 256, and 128, with that size as first arg.
 * Many of these simply delegate to the appropriate implementation with MM_CALL.
 *
 ******************************************************************************/

// use MM_CALL to call macros with the "MM512_name(vec1, vec2)" convention,
// and define those macros to cover up API differences between SSE2/AVX2/AVX512.
#define MM_CALL(SZ, name, ...)  _MM_CALL(SZ, name, __VA_ARGS__)
#define _MM_CALL(SZ, name, ...) MM##SZ##_##name(__VA_ARGS__)

#define MM_CMP_LT(SZ, A, B)    MM_CALL(SZ, CMP_LT, (A), (B))
#define MM_MBLEND(SZ, M, A, B) MM_CALL(SZ, MBLEND, (M), (A), (B))
#define MM_MIN_PD(SZ, A, B)    MM_CALL(SZ, MIN_PD, (A), (B))
#define MM_ADD(SZ, A, B)       MM_CALL(SZ, ADD, (A), (B))

#define MM_LOAD_DBL(SZ, AREF, IDX) MM_CALL(SZ, LOAD_DBL, AREF, (IDX))
#define MM_SETR(SZ, IDX)           MM_CALL(SZ, SETR, (IDX))

#define MM_PLUSPLUS(SZ, IDX)  (IDX = MM_INCR(SZ, IDX))
#define MM_INCR(BITS, VECTOR) MM_ADD(BITS, VECTOR, MM_INCRBY(BITS))

/* WORD size is assumed to be 64-bit...
 * size_t, long, and double are all assumed to be 64-bit.
 */
#define MM_WORDS(BITS) MM##BITS##_WORDS
#define MM512_WORDS    8
#define MM256_WORDS    4
#define MM128_WORDS    2

// TODO: benchmark conditional INCRBY/OFFSETS vs always OFFSETS
// I'm guessing they'll be the same speed. Although incr uses a constant and
// setr uses a var, that var (idx) is almost certainly being kept in a register.
#define MM_INCRBY(BITS) _MM##BITS##_INCRBY
#define _MM512_INCRBY   _mm512_set1_epi64(8L)
#define _MM256_INCRBY   _mm256_set1_epi64x(4L)
#define _MM128_INCRBY   _mm_set1_epi64x(2L)

#define MM_OFFSETS(BITS) _MM##BITS##_OFFSETS
#define _MM512_OFFSETS   _mm512_setr_epi64(0L, 1L, 2L, 3L, 4L, 5L, 6L, 7L)
#define _MM256_OFFSETS   _mm256_setr_epi64x(0L, 1L, 2L, 3L)
#define _MM128_OFFSETS   _mm_setr_epi64x(0L, 1L)

#define MM_BITS(SZ) MM_CALL(SZ, BITS, ~)
#define MM8_BITS(x) 512
#define MM4_BITS(x) 256
#define MM2_BITS(x) 128

/*******************************************************************************
 *
 * specific implementations for each of 512, 256, and 128
 *
 ******************************************************************************/

#define MM512_LOAD_DBL(AREF, IDX)                                              \
    _mm512_set_pd(AREF(IDX + 7),                                               \
                  AREF(IDX + 6),                                               \
                  AREF(IDX + 5),                                               \
                  AREF(IDX + 4),                                               \
                  AREF(IDX + 3),                                               \
                  AREF(IDX + 2),                                               \
                  AREF(IDX + 1),                                               \
                  AREF(IDX))
#define MM256_LOAD_DBL(AREF, IDX)                                              \
    _mm256_set_pd(AREF(IDX + 3), AREF(IDX + 2), AREF(IDX + 1), AREF(IDX))
#define MM128_LOAD_DBL(AREF, IDX) _mm_set_pd(AREF(IDX + 1), AREF(IDX))

#define MM512_ADD(A, B) _mm512_add_epi64((A), (B))
#define MM256_ADD(A, B) _mm256_add_epi64((A), (B))
#define MM128_ADD(A, B) _mm_add_epi64((A), (B))

#define MM512_SET1(IDX) _mm512_set1_epi64(IDX)
#define MM256_SET1(IDX) _mm256_set1_epi64x(IDX)
#define MM128_SET1(IDX) __mm_set1_epi64x(IDX)

#define MM512_SETR(IDX) MM512_ADD(MM_OFFSETS(512), MM512_SET1(IDX))
#define MM256_SETR(IDX) MM256_ADD(MM_OFFSETS(256), MM256_SET1(IDX))
#define MM128_SETR(IDX) _mm_set_epi64x((IDX) + 1, (IDX));

#define MM512d_EXTRACT(VEC, I) _mm512_extractf64x4_pd((VEC), (I));
#define MM512i_EXTRACT(VEC, I) _mm512_extracti64x4_epi64((VEC), (I));

#define MM256d_EXTRACT(VEC, I) _mm256_extractf128_pd((VEC), (I));
#define MM256i_EXTRACT(VEC, I) _mm256_extracti128_si256((VEC), (I));

#define MM512_CMP_LT(A, B) _mm512_cmp_pd_mask((A), (B), _CMP_LT_OS)
#define MM256_CMP_LT(A, B) _mm256_cmp_pd((A), (B), _CMP_LT_OS)
#define MM128_CMP_LT(A, B) _mm_cmplt_pd((A), (B))

#define MM512_MBLEND          _mm512_mask_blend_epi64
#define MM256_MBLEND(M, A, B) _mm256_blendv_epi8((A), (B), (MINT_T(256))(M))
#define MM128_MBLEND(M, A, B) _mm_blendv_epi8((A), (B), _mm_castpd_si128((M)));

#define MM512_MIN_PD(A, B) _mm512_min_pd((A), (B))
#define MM256_MIN_PD(A, B) _mm256_min_pd((A), (B))
#define MM128_MIN_PD(A, B) _mm_min_pd((A), (B))

/*******************************************************************************
 *
 * Generic vectorized min-index algorithm.
 *
 ******************************************************************************/

#define MIN_IDX_INIT(WORDS)      _MIN_IDX_INIT(MM_BITS(WORDS), WORDS)
#define _MIN_IDX_INIT(SZ, WORDS) __MIN_IDX_INIT(SZ, WORDS)
#define __MIN_IDX_INIT(SZ, WORDS)                                              \
    MINT_T(SZ) minidx##WORDS = MM_SETR(SZ, idx);                               \
    MDBL_T(SZ) minval##WORDS = MM_LOAD_DBL(SZ, VAL_AT, idx);                   \
    MINT_T(SZ) cmpidx##WORDS = minidx##WORDS;                                  \
    idx += WORDS;

#define MIN_IDX_REDUCE_MIN(SZ, minvals, minidxs, cmpvals, cmpidxs)             \
    do {                                                                       \
        MASK(SZ) mask = MM_CMP_LT(SZ, cmpvals, minvals);                       \
        minidxs       = MM_MBLEND(SZ, mask, minidxs, cmpidxs);                 \
        minvals       = MM_MIN_PD(SZ, cmpvals, minvals);                       \
    } while (0)

#define MIN_IDX_FOLD_NEXT(BITS, WORDS, AREF, MINIDX, IDX, ...)                 \
    do {                                                                       \
        MDBL_T(BITS) cmpval##WORDS = MM_LOAD_DBL(BITS, AREF, IDX);             \
        MM_PLUSPLUS(BITS, cmpidx##WORDS);                                      \
        MIN_IDX_REDUCE_MIN(                                                    \
          BITS, minval##WORDS, minidx##WORDS, cmpval##WORDS, cmpidx##WORDS);   \
        IDX += MM_WORDS(BITS);                                                 \
    } while (0)

#define MIN_IDX_REDUCE_TO_HALF(OLD_BITS, OLD_WORDS, NEW_BITS, NEW_WORDS)       \
    MDBL_T(NEW_BITS) minval##NEW_WORDS;                                        \
    MINT_T(NEW_BITS) minidx##NEW_WORDS, cmpidx##NEW_WORDS;                     \
    do {                                                                       \
        MDBL_T(NEW_BITS)                                                       \
        cmpval##NEW_WORDS = MM##OLD_BITS##d_EXTRACT(minval##OLD_WORDS, 1);     \
        minval##NEW_WORDS = MM##OLD_BITS##d_EXTRACT(minval##OLD_WORDS, 0);     \
        cmpidx##NEW_WORDS = MM##OLD_BITS##i_EXTRACT(minidx##OLD_WORDS, 1);     \
        minidx##NEW_WORDS = MM##OLD_BITS##i_EXTRACT(minidx##OLD_WORDS, 0);     \
        MIN_IDX_REDUCE_MIN(NEW_BITS,                                           \
                           minval##NEW_WORDS,                                  \
                           minidx##NEW_WORDS,                                  \
                           cmpval##NEW_WORDS,                                  \
                           cmpidx##NEW_WORDS);                                 \
    } while (0)

#define MIN_IDX_REDUCE_8TO2(...)                                               \
    _MIN_IDX_REDUCE_8TO4(__VA_ARGS__);                                         \
    MIN_IDX_REDUCE_4TO2(__VA_ARGS__);

#define MIN_IDX_REDUCE_8TO1(...)                                               \
    _MIN_IDX_REDUCE_8TO4(__VA_ARGS__);                                         \
    _MIN_IDX_REDUCE_4TO2(__VA_ARGS__);                                         \
    MIN_IDX_REDUCE_2TO1(__VA_ARGS__);

#define MIN_IDX_REDUCE_4TO1(...)                                               \
    _MIN_IDX_REDUCE_4TO2(__VA_ARGS__);                                         \
    MIN_IDX_REDUCE_2TO1(__VA_ARGS__);

/*******************************************************************************
 *
 * Specific implementations of the algorithm for each vector size.
 *
 ******************************************************************************/

#define MIN_IDX_INIT_WITH_8(...)  MIN_IDX_INIT(8)
#define MIN_IDX_FOLD_NEXT_8(...)  MIN_IDX_FOLD_NEXT(512, 8, __VA_ARGS__)
#define _MIN_IDX_REDUCE_8TO4(...) MIN_IDX_REDUCE_TO_HALF(512, 8, 256, 4)

#define MIN_IDX_REDUCE_8TO4(AREF, MINIDX, IDX, ...)                            \
    _MIN_IDX_REDUCE_8TO4(AREF, MINIDX, IDX, __VA_ARGS__);                      \
    do {                                                                       \
        /* cmpidxs point to *last* version..., will be incremented later */    \
        /* TODO: benchmark using SETR instead of INCR inside FOLD */           \
        cmpidx4 = MM_SETR(256, IDX - 4);                                       \
    } while (0)

#define MIN_IDX_INIT_WITH_4(...)  MIN_IDX_INIT(4)
#define MIN_IDX_FOLD_NEXT_4(...)  MIN_IDX_FOLD_NEXT(256, 4, __VA_ARGS__)
#define _MIN_IDX_REDUCE_4TO2(...) MIN_IDX_REDUCE_TO_HALF(256, 4, 128, 2)

#define MIN_IDX_REDUCE_4TO2(...)                                               \
    _MIN_IDX_REDUCE_4TO2(__VA_ARGS__);                                         \
    do {                                                                       \
        /* cmpidxs point to *last* version..., will be incremented later */    \
        /* TODO: benchmark using SETR instead of INCR inside FOLD */           \
        cmpidx2 = MM_SETR(128, idx - 2);                                       \
    } while (0)

#define MIN_IDX_INIT_WITH_2(...) MIN_IDX_INIT(2)
#define MIN_IDX_FOLD_NEXT_2(...) MIN_IDX_FOLD_NEXT(128, 2, __VA_ARGS__)

#define MIN_IDX_REDUCE_2TO1(AREF, MINIDX, ...)                                 \
    do {                                                                       \
        double minvals[2];                                                     \
        _mm_storeu_pd(minvals, minval2);                                       \
        MINIDX = ((minvals[0] < minvals[1]) ? _mm_extract_epi64(minidx2, 0)    \
                                            : _mm_extract_epi64(minidx2, 1));  \
    } while (0)

#define MIN_IDX_FOLD_NEXT_1(AREF, MINIDX, IDX, ...)                            \
    do {                                                                       \
        if (AREF(IDX) < AREF(MINIDX)) MINIDX = IDX;                            \
        IDX++;                                                                 \
    } while (0)

/*******************************************************************************
 *
 * Fallback implementations when one of the extensions isn't available.
 *
 ******************************************************************************/

// AVX512F disabled, AVX2 enabled, SSE2 enabled
// AVX512F disabled, AVX2 disabled, SSE2 enabled
// AVX512F disabled, AVX2 disabled, SSE2 disabled
#ifndef DHEAP_ENABLE_MIN_IDX_AVX512
#    undef MIN_IDX_INIT_WITH_8
#    undef MIN_IDX_FOLD_NEXT_8
#    undef MIN_IDX_REDUCE_8TO4
#    undef _MIN_IDX_REDUCE_8TO4
#    define MIN_IDX_INIT_WITH_8(...)                                           \
        MIN_IDX_INIT_WITH_4(__VA_ARGS__);                                      \
        MIN_IDX_FOLD_NEXT_4(__VA_ARGS__);
#    define MIN_IDX_FOLD_NEXT_8(...)                                           \
        do {                                                                   \
            MIN_IDX_FOLD_NEXT_4(__VA_ARGS__);                                  \
            MIN_IDX_FOLD_NEXT_4(__VA_ARGS__);                                  \
        } while (0)
#    define MIN_IDX_REDUCE_8TO4(...)  /* noop */
#    define _MIN_IDX_REDUCE_8TO4(...) /* noop */
#endif

// AVX512F disabled, AVX2 disabled, SSE2 enabled
// AVX512F disabled, AVX2 disabled, SSE2 disabled
#ifndef DHEAP_ENABLE_MIN_IDX_AVX2
#    undef MIN_IDX_INIT_WITH_4
#    undef MIN_IDX_FOLD_NEXT_4
#    undef MIN_IDX_REDUCE_4TO2
#    undef _MIN_IDX_REDUCE_4TO2
#    define MIN_IDX_INIT_WITH_4(...)                                           \
        MIN_IDX_INIT_WITH_2(__VA_ARGS__);                                      \
        MIN_IDX_FOLD_NEXT_2(__VA_ARGS__);
#    define MIN_IDX_FOLD_NEXT_4(...)                                           \
        do {                                                                   \
            MIN_IDX_FOLD_NEXT_2(__VA_ARGS__);                                  \
            MIN_IDX_FOLD_NEXT_2(__VA_ARGS__);                                  \
        } while (0)
#    define MIN_IDX_REDUCE_4TO2(...)  /* noop */
#    define _MIN_IDX_REDUCE_4TO2(...) /* noop */
#endif

// AVX512F disabled, AVX2 disabled, SSE2 disabled
//
// No actual SIMD used here, but we still have the big manually unrolled loops!
// (I'd be astonished if this is faster than "gcc -O3".)
#ifndef DHEAP_ENABLE_MIN_IDX_SSE2
#    undef MIN_IDX_INIT_WITH_2
#    undef MIN_IDX_FOLD_NEXT_2
#    undef MIN_IDX_REDUCE_2TO1
#    define MIN_IDX_INIT_WITH_2(AREF, MINIDX, IDX, ...)                        \
        MINIDX = IDX++;                                                        \
        MIN_IDX_FOLD_NEXT_1(AREF, MINIDX, IDX, __VA_ARGS__);
#    define MIN_IDX_FOLD_NEXT_2(...)                                           \
        do {                                                                   \
            MIN_IDX_FOLD_NEXT_1(__VA_ARGS__);                                  \
            MIN_IDX_FOLD_NEXT_1(__VA_ARGS__);                                  \
        } while (0)
#    define MIN_IDX_REDUCE_2TO1() /* noop */
#endif

/*******************************************************************************
 *
 * Debug printf helpers
 *
 ******************************************************************************/

#ifdef DHEAP_DEBUG

#    include <stdint.h>
#    include <stdio.h>
#    include <string.h>

static inline void
debug_print_m128i64(const char *const label, const MINT_T(128) vector)
{
    size_t val[2];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5ld %5ld\n", label, val[0], val[1]);
}

static inline void
debug_print_m256i64(const char *const label, const MINT_T(256) vector)
{
    size_t val[4];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5ld %5ld %5ld %5ld\n", label, val[0], val[1], val[2], val[3]);
}

static inline void
debug_print_m128_pd(const char *const label, const MDBL_T(128) vector)
{
    double val[2];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5g %5g\n", label, val[0], val[1]);
}

static inline void
debug_print_m256_pd(const char *const label, const MDBL_T(256) vector)
{
    double val[4];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5g %5g %5g %5g\n", label, val[0], val[1], val[2], val[3]);
}

#endif

/*******************************************************************************
 *
 * Yes... that was over 500 lines of complex C preprocessor macros for what is
 * fundamentally three lines of a simple "for" loop.
 *
 ******************************************************************************/
