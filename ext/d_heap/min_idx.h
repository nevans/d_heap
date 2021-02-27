#pragma once

/*
 * This should be defined outside of this file... it should be sent in as a
 * "parameter" to MIN_IDX or SIFT_DOWN.
 */
#define VAL_AT(IDX) entries[(IDX)].score

#ifdef DHEAP_SIMD_ENABLED

// TODO: easy workarounds for these. Need to fix MIN_IDX_REDUCE_HALF, etc
#    if DHEAP_ENABLE_MIN_IDX_AVX512 && !DHEAP_ENABLE_MIN_IDX_AVX2
#        error "Must enable AVX2 when AVX512F is enabled."
#    endif
#    if DHEAP_ENABLE_MIN_IDX_AVX512 && !DHEAP_ENABLE_MIN_IDX_SSE2
#        error "Must enable SSE2 when AVX512F is enabled."
#    endif
#    if DHEAP_ENABLE_MIN_IDX_AVX2 && !DHEAP_ENABLE_MIN_IDX_SSE2
#        error "Must enable SSE2 when AVX2 is enabled."
#    endif

#    include "min_idx_simd.h"
#else /* DHEAP_SIMD_ENABLED */

#    ifdef DHEAP_ENABLE_MIN_IDX_AVX512
#        error "Must set DHEAP_SIMD_ENABLED to use AVX512F"
#    endif

#    if defined(DHEAP_ENABLE_MIN_IDX_AVX512) ||                                \
      defined DHEAP_ENABLE_MIN_IDX_AVX2 || defined DHEAP_ENABLE_MIN_IDX_SSE2
#        error "Must set DHEAP_SIMD_ENABLED to use AVX512F"
#    endif

#    if defined(DHEAP_ENABLE_MIN_IDX_AVX512) ||                                \
      defined DHEAP_ENABLE_MIN_IDX_AVX2 || defined DHEAP_ENABLE_MIN_IDX_SSE2
#        error "Must set DHEAP_SIMD_ENABLED to use AVX512F"
#    endif

#    define MIN_IDX_FAST_PATH(AREF, MIN, IDX, LAST, D) /* noop */
#    define MIN_IDX_COND_PATH(AREF, MIN, IDX, LAST)                            \
        do {                                                                   \
            for (IDX++; IDX <= LAST; IDX++) {                                  \
                if (AREF(IDX) < AREF(MIN)) MIN = IDX;                          \
            }                                                                  \
        } while (0)

#endif

#define MIN_IDX(AREF, IDX, LAST, D)                                            \
    do {                                                                       \
        size_t minidx = IDX;                                                   \
        MIN_IDX_FAST_PATH(AREF, minidx, IDX, LAST, D);                         \
        MIN_IDX_COND_PATH(AREF, minidx, IDX, LAST);                            \
        return minidx;                                                         \
    } while (0)
