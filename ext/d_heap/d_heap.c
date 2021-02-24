#include "ruby.h"
#include <float.h>
#include <math.h>

#ifdef DHEAP_SIMD_ENABLED
#    include <immintrin.h>
#endif

#if CHAR_BIT != 8
#    error "DHeap assumes 8-bit bytes"
#endif

#if SIZE_MAX != ULONG_MAX
#    error "DHeap assumes 'size_t' is 'unsigned long'"
#endif

#if LDBL_MANT_DIG < SIZEOF_UNSIGNED_LONG_LONG * 8
#    error "DHeap assumes 'long double' mantissa can store 'unsigned long long'"
#endif

// TODO: test this code on a 32 bit system (it MUST have some bugs!)
// TODO: perhaps just convert from "unsigned long long" to "uint64_t"!
#if SIZEOF_UNSIGNED_LONG_LONG * 8 != 64
#    error "DHeap assumes 64-bit 'unsigned long long'"
#endif

// TODO: easy workarounds for these:
#if DHEAP_ENABLE_MIN_IDX_AVX512 && !DHEAP_ENABLE_MIN_IDX_AVX2
#    error "Must enable AVX2 when AVX512F is enabled."
#endif
#if DHEAP_ENABLE_MIN_IDX_AVX512 && !DHEAP_ENABLE_MIN_IDX_SSE2
#    error "Must enable SSE2 when AVX512F is enabled."
#endif
#if DHEAP_ENABLE_MIN_IDX_AVX2 && !DHEAP_ENABLE_MIN_IDX_SSE2
#    error "Must enable SSE2 when AVX2 is enabled."
#endif

/********************************************************************
 *
 * Type definitions
 *
 ********************************************************************/

typedef struct dheap_struct dheap_t;
typedef struct dheap_entry  ENTRY;

typedef double SCORE;

/********************************************************************
 *
 * Struct definitions
 *
 ********************************************************************/

struct dheap_struct
{
    int    d;
    size_t size;
    size_t capa;
    ENTRY *entries;
#ifdef DHEAP_MAP
    VALUE indexes; // Hash
#endif
};

struct dheap_entry
{
    SCORE score;
    VALUE value;
};

#define DHEAPMAP_P(heap) UNLIKELY(RTEST((heap)->indexes))

/********************************************************************
 *
 * Constant definitions
 *
 ********************************************************************/

#define DHEAP_DEFAULT_D 12
#define DHEAP_MAX_D     INT_MAX

// sizeof(ENTRY) => 16 bytes, 128-bits
// one kilobyte = 32 * 32 bytes
#define DHEAP_DEFAULT_CAPA  32
#define DHEAP_MAX_CAPA      (SIZE_MAX / (int)sizeof(ENTRY))
#define DHEAP_CAPA_INCR_MAX (10 * 1024 * 1024 / (int)sizeof(ENTRY))

#ifdef DHEAP_ENABLE_MIN_IDX_AVX512
static __m512i idx64x8, incrby8;
#endif
#ifdef DHEAP_ENABLE_MIN_IDX_AVX2
static __m256i idx64x4, incrby4;
#endif
#ifdef DHEAP_ENABLE_MIN_IDX_SSE2
static __m128i incrby2;
#endif

static ID id_cmp;    // <=>
static ID id_abs;    // abs
static ID id_lshift; // <<
static ID id_uminus; // -@

static const rb_data_type_t dheap_data_type;

/********************************************************************
 *
 * Metaprogramming macros
 *
 ********************************************************************/

#define LIKELY   RB_LIKELY
#define UNLIKELY RB_UNLIKELY

#ifdef DHEAP_MAP
#    define DHEAP_DISPATCH_EXPR(func, heap, ...)                               \
        (DHEAPMAP_P(heap) ? dheapmap_##func(heap, __VA_ARGS__)                 \
                          : dheap_##func(heap, __VA_ARGS__))
#else
#    define DHEAP_DISPATCH_EXPR(func, heap, ...)                               \
        dheap_##func(heap, __VA_ARGS__);
#endif

#ifdef DHEAP_MAP
#    define DHEAP_DISPATCH_STMT(heap, macro, ...)                              \
        do {                                                                   \
            if (DHEAPMAP_P(heap)) {                                            \
                macro(dheapmap, heap, __VA_ARGS__);                            \
            } else {                                                           \
                macro(dheap, heap, __VA_ARGS__);                               \
            }                                                                  \
        } while (0)
#else
#    define DHEAP_DISPATCH_STMT(heap, macro, ...)                              \
        macro(dheap, heap, __VA_ARGS__)
#endif

/********************************************************************
 *
 * SCORE: casting to and from VALUE
 *    adapted from similar methods in ruby's object.c
 *
 ********************************************************************/

#define SCORE2NUM(score) rb_float_new(score)
#define VAL2SCORE(val)   NUM2DBL(RB_FLOAT_TYPE_P(val) ? val : rb_Float(val))
#define CMP_LT(a, b)     ((a) < (b))
#define CMP_LTE(a, b)    ((a) <= (b))

/********************************************************************
 *
 * DHeap ENTRY accessors
 *
 ********************************************************************/

#define DHEAP_SCORE(heap, idx) (DHEAP_GET(heap, idx).score)
#define DHEAP_VALUE(heap, idx) (DHEAP_GET(heap, idx).value)

// idx must be size_t
#define DHEAP_ENTRY_ARY(heap, idx)                                             \
    (((heap)->size <= (idx))                                                   \
       ? Qnil                                                                  \
       : rb_ary_new_from_args(                                                 \
           2, DHEAP_VALUE(heap, idx), SCORE2NUM(DHEAP_SCORE(heap, idx))))

#define DHEAP_GET(heap, idx) ((heap)->entries[idx])
#define DHEAP_SET(T, heap, index, entry)                                       \
    do {                                                                       \
        DHEAP_GET(heap, index) = (entry);                                      \
        DHEAP_SET_##T(heap, index, entry);                                     \
    } while (0)

#define DHEAP_SET_dheap(heap, index, entry) /* noop */

#ifdef DHEAP_MAP
#    define DHEAP_SET_dheapmap(heap, index, entry)                             \
        rb_hash_aset((heap)->indexes, (entry).value, ULONG2NUM(index))
#endif

/********************************************************************
 *
 * DHeap index math
 *
 ********************************************************************/

#define DHEAP_IDX_LAST(heap)         ((heap)->size - 1)
#define DHEAP_IDX_PARENT(heap, idx)  (((idx)-1) / (heap)->d)
#define DHEAP_IDX_CHILD_0(heap, idx) (((idx) * (heap)->d) + 1)
#define DHEAP_IDX_CHILD_D(heap, idx) (((idx) * (heap)->d) + (heap)->d)

#ifdef DHEAP_DEBUG
#    define ASSERT_DHEAP_IDX_OK(heap, index)                                   \
        do {                                                                   \
            if (DHEAP_IDX_LAST(heap) < index) {                                \
                rb_raise(rb_eIndexError, "DHeap index %ld too large", index);  \
            }                                                                  \
        } while (0)
#else
#    define ASSERT_DHEAP_IDX_OK(heap, index)
#endif

/********************************************************************
 *
 * rb_data_type_t definitions
 *
 ********************************************************************/

#ifdef HAVE_RB_GC_MARK_MOVABLE
static void
dheap_compact(void *ptr)
{
    dheap_t *heap = ptr;
    for (size_t i = 0; i < heap->size; ++i) {
        if (DHEAP_VALUE(heap, i))
            DHEAP_VALUE(heap, i) = rb_gc_location(DHEAP_VALUE(heap, i));
    }
#    ifdef DHEAP_MAP
    if (DHEAPMAP_P(heap)) heap->indexes = rb_gc_location(heap->indexes);
#    endif
}
#else
#    define rb_gc_mark_movable(x) rb_gc_mark(x)
#endif

static void
dheap_mark(void *ptr)
{
    dheap_t *heap = ptr;
    for (size_t i = 0; i < heap->size; ++i) {
        if (DHEAP_VALUE(heap, i)) rb_gc_mark_movable(DHEAP_VALUE(heap, i));
    }
#ifdef DHEAP_MAP
    if (DHEAPMAP_P(heap)) rb_gc_mark_movable(heap->indexes);
#endif
}

static void
dheap_free(void *ptr)
{
    dheap_t *heap = ptr;
    heap->size    = 0;
    if (heap->entries) {
        xfree(heap->entries);
        heap->entries = NULL;
    }
    heap->capa = 0;
    xfree(ptr);
#ifdef DHEAP_MAP
    heap->indexes = Qnil;
#endif
}

static size_t
dheap_memsize(const void *ptr)
{
    const dheap_t *heap = ptr;
    size_t         size = 0;
    size += sizeof(*heap);
    size += sizeof(ENTRY) * heap->capa;
    return size;
}

static const rb_data_type_t dheap_data_type = {
    "DHeap",
    { (void (*)(void *))dheap_mark,
      (void (*)(void *))dheap_free,
      (size_t(*)(const void *))dheap_memsize,
#ifdef HAVE_RB_GC_MARK_MOVABLE
      (void (*)(void *))dheap_compact,
      { 0 }
#else
      { 0 }
#endif
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

/********************************************************************
 *
 * DHeap comparisons
 *
 ********************************************************************/

/********************************************************************
 *
 * DHeap allocation and initialization and resizing
 *
 ********************************************************************/

static VALUE
dheap_s_alloc(VALUE klass)
{
    VALUE    obj;
    dheap_t *heap;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    // TypedData_Make_Struct uses a non-std "statement expression"
    obj = TypedData_Make_Struct(klass, dheap_t, &dheap_data_type, heap);
#pragma GCC diagnostic pop
    heap->d       = DHEAP_DEFAULT_D;
    heap->size    = 0;
    heap->capa    = 0;
    heap->entries = NULL;
#ifdef DHEAP_MAP
    heap->indexes = Qnil;
#endif

    return obj;
}

static inline dheap_t *
get_dheap_struct(VALUE self)
{
    dheap_t *heap;
    TypedData_Get_Struct(self, dheap_t, &dheap_data_type, heap);
    return heap;
}

static inline dheap_t *
get_dheap_struct_unfrozen(VALUE self)
{
    rb_check_frozen(self);
    return get_dheap_struct(self);
}

void
dheap_set_capa(dheap_t *heap, size_t new_capa)
{
    // Do nothing if we already have the capacity or are resizing too small
    if (new_capa <= heap->capa || new_capa <= heap->size) return;

    // allocate
    if (heap->entries) {
        RB_REALLOC_N(heap->entries, ENTRY, new_capa);
    } else {
        heap->entries = RB_ZALLOC_N(ENTRY, new_capa);
    }
    heap->capa = new_capa;
}

static void
dheap_incr_capa(dheap_t *heap, size_t new_size)
{
    size_t new_capa = heap->capa;
    while (new_capa < new_size) {
        if (new_capa <= DHEAP_CAPA_INCR_MAX) {
            // double it... up to DHEAP_CAPA_INCR_MAX
            new_capa *= 2;
        } else if (DHEAP_MAX_CAPA - DHEAP_CAPA_INCR_MAX < heap->capa) {
            // avoid overflow
            new_capa = DHEAP_MAX_CAPA;
        } else {
            new_capa += DHEAP_CAPA_INCR_MAX;
        }
    }
    dheap_set_capa(heap, new_capa);
}

static void
dheap_ensure_room_for_push(dheap_t *heap, size_t incr_by)
{
    // check for overflow of new_size
    if (DHEAP_MAX_CAPA - incr_by < heap->size) {
        rb_raise(rb_eIndexError,
                 "size increase overflow: %zu + %zu",
                 heap->size,
                 incr_by);
    } else {
        size_t new_size = heap->size + incr_by;
        // if existing capacity is too small
        if (heap->capa < new_size) dheap_incr_capa(heap, new_size);
    }
}

static inline int
dheap_value_to_int_d(VALUE num)
{
    int d = NUM2INT(num);
    if (d < 2) rb_raise(rb_eArgError, "DHeap d=%u is too small", d);
    if (d > DHEAP_MAX_D) rb_raise(rb_eArgError, "DHeap d=%u is too large", d);
    return d;
}

static inline size_t
dheap_value_to_capa(VALUE num)
{
    size_t capa = NUM2ULONG(num);
    if (capa < 1) {
        rb_raise(rb_eArgError, "DHeap capa=%zu must be positive", capa);
    }
    return capa;
}

static VALUE
dheap_init(VALUE self, VALUE d, VALUE capa, VALUE map)
{
    dheap_t *heap = get_dheap_struct(self);

    if (heap->entries || heap->size || heap->capa)
        rb_raise(rb_eScriptError, "DHeap already initialized.");

    heap->d = dheap_value_to_int_d(d);
    dheap_set_capa(heap, dheap_value_to_capa(capa));
#ifdef DHEAP_MAP
    if (RTEST(map)) heap->indexes = rb_hash_new();
#endif

    return self;
}

/* @!visibility private */
static VALUE
dheap_initialize_copy(VALUE copy, VALUE orig)
{
    dheap_t *heap_copy = get_dheap_struct_unfrozen(copy);
    dheap_t *heap_orig = get_dheap_struct(orig);

    heap_copy->d = heap_orig->d;

    dheap_set_capa(heap_copy, heap_orig->capa);
    heap_copy->size = heap_orig->size;
    if (heap_copy->size)
        MEMCPY(heap_copy->entries, heap_orig->entries, ENTRY, heap_orig->size);
#ifdef DHEAP_MAP
    if (RTEST(heap_orig->indexes))
        heap_copy->indexes = rb_hash_dup(heap_orig->indexes);
#endif

    return copy;
}

/********************************************************************
 *
 * DHeap sift up/down
 *
 ********************************************************************/

#define DHEAP_SIFT_UP(T, heap, i)                                              \
    do {                                                                       \
        size_t sift_idx = i;                                                   \
        ENTRY  entry    = DHEAP_GET(heap, sift_idx);                           \
        ASSERT_DHEAP_IDX_OK(heap, sift_idx);                                   \
        for (size_t parent_idx; 0 < sift_idx; sift_idx = parent_idx) {         \
            parent_idx = DHEAP_IDX_PARENT(heap, sift_idx);                     \
            if (CMP_LTE(DHEAP_SCORE((heap), parent_idx), entry.score)) break;  \
            DHEAP_SET(T, heap, sift_idx, DHEAP_GET(heap, parent_idx));         \
        }                                                                      \
        DHEAP_SET(T, heap, sift_idx, entry);                                   \
    } while (0)

#ifdef DHEAP_SIMD_ENABLED

#    ifdef DHEAP_DEBUG

void
debug_print_m128i64(const char *const label, const __m128i vector)
{
    uint64_t val[2];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5ld %5ld\n", label, val[0], val[1]);
}

void
debug_print_m256i64(const char *const label, const __m256i vector)
{
    uint64_t val[4];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5ld %5ld %5ld %5ld\n", label, val[0], val[1], val[2], val[3]);
}

void
debug_print_m128_pd(const char *const label, const __m128d vector)
{
    double val[2];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5g %5g\n", label, val[0], val[1]);
}

void
debug_print_m256_pd(const char *const label, const __m256d vector)
{
    double val[4];
    memcpy(val, &vector, sizeof(val));
    printf("%s: %5g %5g %5g %5g\n", label, val[0], val[1], val[2], val[3]);
}

void
debug_print_dheap(const dheap_t *const heap, int inspect)
{
    printf("#<DHeap d=%d size=%zd", heap->d, heap->size);
    if (inspect) {
        for (size_t i = 0; i < (size_t)heap->size; i++) {
            if (i % heap->d == 1)
                printf("   (p=%zd)", DHEAP_IDX_PARENT(heap, i));
            printf(" [%zd]=%g", i, DHEAP_SCORE(heap, i));
            if (DHEAP_SCORE(heap, i) <
                DHEAP_SCORE(heap, DHEAP_IDX_PARENT(heap, i)))
                printf("<-!!!!!!!!!");
        }
    }
    printf(">");
}

#    endif

#    define _MM512_LOAD_SCORES(entries, idx)                                   \
        _mm512_set_pd(entries[idx + 7].score,                                  \
                      entries[idx + 6].score,                                  \
                      entries[idx + 5].score,                                  \
                      entries[idx + 4].score,                                  \
                      entries[idx + 3].score,                                  \
                      entries[idx + 2].score,                                  \
                      entries[idx + 1].score,                                  \
                      entries[idx].score)

#    define _MM256_LOAD_SCORES(entries, idx)                                   \
        _mm256_set_pd(entries[idx + 3].score,                                  \
                      entries[idx + 2].score,                                  \
                      entries[idx + 1].score,                                  \
                      entries[idx].score)

#    define _MM256_LOAD_SCORES_OVERFLOW(entries, idx)                          \
        _mm256_set_pd(entries[idx].score,                                      \
                      entries[idx + 2].score,                                  \
                      entries[idx + 1].score,                                  \
                      entries[idx].score)

#    define _MM256_SETR_INDEXES(idx)                                           \
        _mm256_add_epi64(idx64x4, _mm256_set1_epi64x(idx));

#    define _MM128_LOAD_SCORES(entries, idx)                                   \
        _mm_set_pd((entries)[idx + 1].score, (entries)[idx].score)

#    define _MM512_SETR_INDEXES(idx)                                           \
        _mm512_add_epi64(idx64x8, _mm512_set1_epi64(idx));

#    define _MM512_REDUCE_MIN(val0, idx0, val1, idx1)                          \
        do {                                                                   \
            const __mmask8 ltmask8 = _mm512_cmplt_pd_mask(cmpval8, minval8);   \
            idx0 = _mm512_mask_blend_epi64(ltmask8, idx0, idx1);               \
            val0 = _mm512_min_pd(val1, val0);                                  \
        } while (0)

#    define _MM256_REDUCE_MIN(val0, idx0, val1, idx1)                          \
        do {                                                                   \
            const __m256d ltmask4 = _mm256_cmp_pd(val1, val0, _CMP_LT_OS);     \
            idx0 = _mm256_blendv_epi8(idx0, idx1, (__m256i)ltmask4);           \
            val0 = _mm256_min_pd(val1, val0);                                  \
        } while (0)

#    define _MM128_REDUCE_MIN(val0, idx0, val1, idx1)                          \
        do {                                                                   \
            const __m128d ltmask2 = _mm_cmplt_pd(val1, val0);                  \
            idx0 = _mm_blendv_epi8(idx0, idx1, _mm_castpd_si128(ltmask2));     \
            val0 = _mm_min_pd(val1, val0);                                     \
        } while (0)

#    define SCORE_AT(i)   DHEAP_SCORE(heap, i)
#    define MINIDX2(i, j) (SCORE_AT(i) < SCORE_AT(j) ? i : j)
#    define MINIDX3(i, j, k)                                                   \
        (SCORE_AT(i) < SCORE_AT(j) ? MINIDX2(i, k) : MINIDX2(j, k))

/*
 * funrolling loops by hand is fun!
 *
 * Seriously though, although this might be more than a little bit crazy, in its
 * own way it's a lot _simpler_ and easier to reason about than a bunch of loops
 * and conditionals. No loops. No conditions. Just connect the dots.
 */

/* clang-format off */
#define MIN_IDX_REDUCE_SIZE32()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX24_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE31()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX23_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE30()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX22_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE29()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX21_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE28()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX20_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE27()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX19_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE26()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX18_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE25()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX17_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE24()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX16_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE23()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX15_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE22()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX14_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE21()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX13_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE20()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX12_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE19()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX11_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE18()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX10_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE17()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX09_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE16()     do { MIN_IDX_INIT_WITH_8();  MIN_IDX_REDUCE_IDX08_VEC8(); } while (0)
#define MIN_IDX_REDUCE_SIZE15()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX11_VEC4(); } while (0)
#define MIN_IDX_REDUCE_SIZE14()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX10_VEC4(); } while (0)
#define MIN_IDX_REDUCE_SIZE13()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX09_VEC4(); } while (0)

#define MIN_IDX_REDUCE_SIZE12()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX08_VEC4(); } while (0)
#define MIN_IDX_REDUCE_SIZE11()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX07_VEC4(); } while (0)
#define MIN_IDX_REDUCE_SIZE10()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX06_VEC4(); } while (0)
#define MIN_IDX_REDUCE_SIZE09()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX05_VEC4(); } while (0)
#define MIN_IDX_REDUCE_SIZE08()     do { MIN_IDX_INIT_WITH_4();  MIN_IDX_REDUCE_IDX04_VEC4(); } while (0)
#define MIN_IDX_REDUCE_SIZE07()     do { MIN_IDX_INIT_WITH_2();  MIN_IDX_REDUCE_IDX05_VEC2(); } while (0)
#define MIN_IDX_REDUCE_SIZE06()     do { MIN_IDX_INIT_WITH_2();  MIN_IDX_REDUCE_IDX04_VEC2(); } while (0)
#define MIN_IDX_REDUCE_SIZE05()     do { MIN_IDX_INIT_WITH_2();  MIN_IDX_REDUCE_IDX03_VEC2(); } while (0)
#define MIN_IDX_REDUCE_SIZE04()     do { MIN_IDX_INIT_WITH_2();  MIN_IDX_REDUCE_IDX02_VEC2(); } while (0)

#define MIN_IDX_REDUCE_SIZE03()     do { minidx = MINIDX3(idx, idx + 1, idx + 2); } while (0)
#define MIN_IDX_REDUCE_SIZE02()     do { minidx = MINIDX2(idx, idx + 1); } while (0)
#define MIN_IDX_REDUCE_SIZE01()     do { minidx = idx; } while (0)

#define MIN_IDX_REDUCE_IDX24_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX16_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX23_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX15_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX22_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX14_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX21_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX13_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX20_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX12_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX19_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX11_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX18_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX10_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX17_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX09_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX16_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX08_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX15_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX07_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX14_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX06_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX13_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX05_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX12_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX04_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX11_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX03_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX10_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX02_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX09_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX01_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX08_VEC8() do { MIN_IDX_FOLD_NEXT_8();  MIN_IDX_REDUCE_IDX00_VEC8(); } while (0)
#define MIN_IDX_REDUCE_IDX07_VEC8() do { MIN_IDX_REDUCE_8TO4(1); MIN_IDX_REDUCE_IDX07_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX06_VEC8() do { MIN_IDX_REDUCE_8TO4(1); MIN_IDX_REDUCE_IDX06_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX05_VEC8() do { MIN_IDX_REDUCE_8TO4(1); MIN_IDX_REDUCE_IDX05_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX04_VEC8() do { MIN_IDX_REDUCE_8TO4(1); MIN_IDX_REDUCE_IDX04_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX03_VEC8() do { MIN_IDX_REDUCE_8TO4(0); MIN_IDX_REDUCE_IDX03_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC8() do { MIN_IDX_REDUCE_8TO4(0); MIN_IDX_REDUCE_IDX02_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX01_VEC8() do { MIN_IDX_REDUCE_8TO4(0); MIN_IDX_REDUCE_IDX01_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC8() do { MIN_IDX_REDUCE_8TO4(0); MIN_IDX_REDUCE_IDX00_VEC4(); } while (0)

#define MIN_IDX_REDUCE_IDX11_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX07_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX10_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX06_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX09_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX05_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX08_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX04_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX07_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX03_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX06_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX02_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX05_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX01_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX04_VEC4() do { MIN_IDX_FOLD_NEXT_4();  MIN_IDX_REDUCE_IDX00_VEC4(); } while (0)
#define MIN_IDX_REDUCE_IDX03_VEC4() do { MIN_IDX_REDUCE_4TO2(1); MIN_IDX_REDUCE_IDX03_VEC2(); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC4() do { MIN_IDX_REDUCE_4TO2(1); MIN_IDX_REDUCE_IDX02_VEC2(); } while (0)
#define MIN_IDX_REDUCE_IDX01_VEC4() do { MIN_IDX_REDUCE_4TO2(0); MIN_IDX_REDUCE_IDX01_VEC2(); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC4() do { MIN_IDX_REDUCE_4TO2(0); MIN_IDX_REDUCE_IDX00_VEC2(); } while (0)

#define MIN_IDX_REDUCE_IDX05_VEC2() do { MIN_IDX_FOLD_NEXT_2();  MIN_IDX_REDUCE_IDX03_VEC2(); } while (0)
#define MIN_IDX_REDUCE_IDX04_VEC2() do { MIN_IDX_FOLD_NEXT_2();  MIN_IDX_REDUCE_IDX02_VEC2(); } while (0)
#define MIN_IDX_REDUCE_IDX03_VEC2() do { MIN_IDX_FOLD_NEXT_2();  MIN_IDX_REDUCE_IDX01_VEC2(); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC2() do { MIN_IDX_FOLD_NEXT_2();  MIN_IDX_REDUCE_IDX00_VEC2(); } while (0)
#define MIN_IDX_REDUCE_IDX01_VEC2() do { MIN_IDX_REDUCE_2TO1();  MIN_IDX_REDUCE_IDX01_VEC1(); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC2() do { MIN_IDX_REDUCE_2TO1();  MIN_IDX_REDUCE_IDX00_VEC1(); } while (0)

#define MIN_IDX_REDUCE_IDX04_VEC1() do { MIN_IDX_FOLD_NEXT_1();  MIN_IDX_REDUCE_IDX03_VEC1(); } while (0)
#define MIN_IDX_REDUCE_IDX03_VEC1() do { MIN_IDX_FOLD_NEXT_1();  MIN_IDX_REDUCE_IDX02_VEC1(); } while (0)
#define MIN_IDX_REDUCE_IDX02_VEC1() do { MIN_IDX_FOLD_NEXT_1();  MIN_IDX_REDUCE_IDX01_VEC1(); } while (0)
#define MIN_IDX_REDUCE_IDX01_VEC1() do { MIN_IDX_FOLD_NEXT_1();  MIN_IDX_REDUCE_IDX00_VEC1(); } while (0)
#define MIN_IDX_REDUCE_IDX00_VEC1() /* done; result is in minidx */
/* clang-format on */

#    define MIN_IDX_REDUCE_SIZE_D(d)  _MIN_IDX_REDUCE_SIZE_D(d)
#    define _MIN_IDX_REDUCE_SIZE_D(d) MIN_IDX_REDUCE_SIZE##d()
#    define MIN_IDX_REDUCE_SIZE1(d)   MIN_IDX_REDUCE_SIZE01()
#    define MIN_IDX_REDUCE_SIZE2(d)   MIN_IDX_REDUCE_SIZE02()
#    define MIN_IDX_REDUCE_SIZE3(d)   MIN_IDX_REDUCE_SIZE03()
#    define MIN_IDX_REDUCE_SIZE4(d)   MIN_IDX_REDUCE_SIZE04()
#    define MIN_IDX_REDUCE_SIZE5(d)   MIN_IDX_REDUCE_SIZE05()
#    define MIN_IDX_REDUCE_SIZE6(d)   MIN_IDX_REDUCE_SIZE06()
#    define MIN_IDX_REDUCE_SIZE7(d)   MIN_IDX_REDUCE_SIZE07()
#    define MIN_IDX_REDUCE_SIZE8(d)   MIN_IDX_REDUCE_SIZE08()
#    define MIN_IDX_REDUCE_SIZE9(d)   MIN_IDX_REDUCE_SIZE09()

#    ifdef DHEAP_ENABLE_MIN_IDX_AVX512

#        define MIN_IDX_INIT_WITH_8()                                          \
            __m128d minval2;                                                   \
            __m128i minidx2, cmpidx2;                                          \
            __m256d minval4;                                                   \
            __m256i minidx4, cmpidx4;                                          \
            __m512i minidx8 = _MM512_SETR_INDEXES(idx);                        \
            __m512d minval8 = _MM512_LOAD_SCORES(entries, idx);                \
            __m512i cmpidx8 = minidx8;                                         \
            idx += 8;

#        define MIN_IDX_FOLD_NEXT_8()                                          \
            do {                                                               \
                const __m512d cmpval8 = _MM512_LOAD_SCORES(entries, idx);      \
                cmpidx8               = _mm512_add_epi32(cmpidx8, incrby8);    \
                _MM512_REDUCE_MIN(minval8, minidx8, cmpval8, cmpidx8);         \
                idx += 8;                                                      \
            } while (0)

// TODO: AVX512 enabled, AVX2 disabled, SSE2 enabled
// TODO: AVX512 enabled, AVX2 disabled, SSE2 disabled
#        define MIN_IDX_REDUCE_8TO4(reset_cmpidx4)                             \
            do {                                                               \
                minval4               = _mm512_extractf64x4_pd(minval8, 0);    \
                const __m256d cmpval4 = _mm512_extractf64x4_pd(minval8, 1);    \
                minidx4               = _mm512_extracti64x4_epi64(minidx8, 0); \
                cmpidx4               = _mm512_extracti64x4_epi64(minidx8, 1); \
                _MM256_REDUCE_MIN(minval4, minidx4, cmpval4, cmpidx4);         \
                cmpidx4 = _MM256_SETR_INDEXES(idx - 4);                        \
            } while (0)

#    else

#        define MIN_IDX_INIT_WITH_8()                                          \
            MIN_IDX_INIT_WITH_4();                                             \
            MIN_IDX_FOLD_NEXT_4();

#        define MIN_IDX_FOLD_NEXT_8()                                          \
            do {                                                               \
                MIN_IDX_FOLD_NEXT_4();                                         \
                MIN_IDX_FOLD_NEXT_4();                                         \
            } while (0)

#        define MIN_IDX_REDUCE_8TO4(reset_cmpidx4) /* noop */

#    endif

#    ifdef DHEAP_ENABLE_MIN_IDX_AVX2

#        define MIN_IDX_INIT_WITH_4()                                          \
            __m128d minval2;                                                   \
            __m128i minidx2, cmpidx2;                                          \
            __m256i minidx4 = _MM256_SETR_INDEXES(idx);                        \
            __m256d minval4 = _MM256_LOAD_SCORES(entries, idx);                \
            __m256i cmpidx4 = minidx4;                                         \
            idx += 4;

#        define MIN_IDX_FOLD_NEXT_4()                                          \
            do {                                                               \
                const __m256d cmpval4 = _MM256_LOAD_SCORES(entries, idx);      \
                cmpidx4               = _MM256_SETR_INDEXES(idx);              \
                _MM256_REDUCE_MIN(minval4, minidx4, cmpval4, cmpidx4);         \
                idx += 4;                                                      \
            } while (0)

// TODO: AVX2 enabled, SSE2 disabled
#        define MIN_IDX_REDUCE_4TO2(reset_cmpidx2)                             \
            do {                                                               \
                minval2               = _mm256_extractf128_pd(minval4, 0);     \
                const __m128d cmpval2 = _mm256_extractf128_pd(minval4, 1);     \
                minidx2               = _mm256_extracti128_si256(minidx4, 0);  \
                cmpidx2               = _mm256_extracti128_si256(minidx4, 1);  \
                _MM128_REDUCE_MIN(minval2, minidx2, cmpval2, cmpidx2);         \
            } while (0)

#    else

#        define MIN_IDX_INIT_WITH_4()                                          \
            MIN_IDX_INIT_WITH_2();                                             \
            MIN_IDX_FOLD_NEXT_2();

#        define MIN_IDX_FOLD_NEXT_4()                                          \
            do {                                                               \
                MIN_IDX_FOLD_NEXT_2();                                         \
                MIN_IDX_FOLD_NEXT_2();                                         \
            } while (0)

#        define MIN_IDX_REDUCE_4TO2(reset_cmpidx2) /* noop */

#    endif

#    ifdef DHEAP_ENABLE_MIN_IDX_SSE2

#        define MIN_IDX_INIT_WITH_2()                                          \
            __m128i minidx2 = _mm_set_epi64x(idx + 1, idx);                    \
            __m128d minval2 = _MM128_LOAD_SCORES(entries, idx);                \
            __m128i cmpidx2 = minidx2;                                         \
            idx += 2;

#        define MIN_IDX_FOLD_NEXT_2()                                          \
            do {                                                               \
                const __m128d cmpval2 = _MM128_LOAD_SCORES(entries, idx);      \
                cmpidx2               = _mm_set_epi64x(idx + 1, idx);          \
                _MM128_REDUCE_MIN(minval2, minidx2, cmpval2, cmpidx2);         \
                idx += 2;                                                      \
            } while (0)

#        define MIN_IDX_REDUCE_2TO1()                                          \
            do {                                                               \
                double minvals[2];                                             \
                _mm_storeu_pd(minvals, minval2);                               \
                minidx =                                                       \
                  ((minvals[0] < minvals[1]) ? _mm_extract_epi64(minidx2, 0)   \
                                             : _mm_extract_epi64(minidx2, 1)); \
            } while (0)

#    else

#        define MIN_IDX_INIT_WITH_2()                                          \
            minidx = idx++;                                                    \
            MIN_IDX_FOLD_NEXT_1();

#        define MIN_IDX_FOLD_NEXT_2()                                          \
            do {                                                               \
                MIN_IDX_FOLD_NEXT_1();                                         \
                MIN_IDX_FOLD_NEXT_1();                                         \
            } while (0)

#    endif

#    define MIN_IDX_FOLD_NEXT_1()                                              \
        do {                                                                   \
            if (entries[idx].score < entries[minidx].score) minidx = idx;      \
            idx++;                                                             \
        } while (0)

static inline size_t
min_index_simd(dheap_t *heap, size_t idx, const size_t last)
{
    const ENTRY *entries = heap->entries;
    size_t       minidx;

    // short-circuit for the default
    if (last - idx == DHEAP_DEFAULT_D - 1) {
        MIN_IDX_REDUCE_SIZE_D(DHEAP_DEFAULT_D);
        return minidx;
    }

    switch (last - idx) {
        /* clang-format off */

    // handled without vectors
    case 0: MIN_IDX_REDUCE_SIZE01(); return minidx;
    case 1: MIN_IDX_REDUCE_SIZE02(); return minidx;
    case 2: MIN_IDX_REDUCE_SIZE03(); return minidx;

    // uses 128-bit vectors
    case 3: MIN_IDX_REDUCE_SIZE04(); return minidx;
    case 4: MIN_IDX_REDUCE_SIZE05(); return minidx;
    case 5: MIN_IDX_REDUCE_SIZE06(); return minidx;
    case 6: MIN_IDX_REDUCE_SIZE07(); return minidx;

    // uses 256-bit vectors
    case 7:  MIN_IDX_REDUCE_SIZE08(); return minidx;
    case 8:  MIN_IDX_REDUCE_SIZE09(); return minidx;
    case 9:  MIN_IDX_REDUCE_SIZE10(); return minidx;
    case 10: MIN_IDX_REDUCE_SIZE11(); return minidx;
    case 11: MIN_IDX_REDUCE_SIZE12(); return minidx;
    case 12: MIN_IDX_REDUCE_SIZE13(); return minidx;
    case 13: MIN_IDX_REDUCE_SIZE14(); return minidx;
    case 14: MIN_IDX_REDUCE_SIZE15(); return minidx;

    // uses 512-bit vectors
    case 15: MIN_IDX_REDUCE_SIZE16(); return minidx;
    case 16: MIN_IDX_REDUCE_SIZE17(); return minidx;
    case 17: MIN_IDX_REDUCE_SIZE18(); return minidx;
    case 18: MIN_IDX_REDUCE_SIZE19(); return minidx;
    case 19: MIN_IDX_REDUCE_SIZE20(); return minidx;
    case 20: MIN_IDX_REDUCE_SIZE21(); return minidx;
    case 21: MIN_IDX_REDUCE_SIZE22(); return minidx;
    case 22: MIN_IDX_REDUCE_SIZE23(); return minidx;
    case 23: MIN_IDX_REDUCE_SIZE24(); return minidx;
    case 24: MIN_IDX_REDUCE_SIZE25(); return minidx;
    case 25: MIN_IDX_REDUCE_SIZE26(); return minidx;
    case 26: MIN_IDX_REDUCE_SIZE27(); return minidx;
    case 27: MIN_IDX_REDUCE_SIZE28(); return minidx;
    case 28: MIN_IDX_REDUCE_SIZE29(); return minidx;
    case 29: MIN_IDX_REDUCE_SIZE30(); return minidx;
    case 30: MIN_IDX_REDUCE_SIZE31(); return minidx;
    case 31: MIN_IDX_REDUCE_SIZE32(); return minidx;

        /* clang-format on */
    default:
        // above 32 loops on 8 at a time, until less than 24 remaining
        do {
            MIN_IDX_INIT_WITH_8();
            while (23 < last - idx) {
                MIN_IDX_FOLD_NEXT_8();
            }
            switch (last - idx) {
                /* clang-format off */
            case 0:  MIN_IDX_REDUCE_IDX01_VEC8(); return minidx;
            case 1:  MIN_IDX_REDUCE_IDX02_VEC8(); return minidx;
            case 2:  MIN_IDX_REDUCE_IDX03_VEC8(); return minidx;
            case 3:  MIN_IDX_REDUCE_IDX04_VEC8(); return minidx;
            case 4:  MIN_IDX_REDUCE_IDX05_VEC8(); return minidx;
            case 5:  MIN_IDX_REDUCE_IDX06_VEC8(); return minidx;
            case 6:  MIN_IDX_REDUCE_IDX07_VEC8(); return minidx;
            case 7:  MIN_IDX_REDUCE_IDX08_VEC8(); return minidx;
            case 8:  MIN_IDX_REDUCE_IDX09_VEC8(); return minidx;
            case 9:  MIN_IDX_REDUCE_IDX10_VEC8(); return minidx;
            case 10: MIN_IDX_REDUCE_IDX11_VEC8(); return minidx;
            case 11: MIN_IDX_REDUCE_IDX12_VEC8(); return minidx;
            case 12: MIN_IDX_REDUCE_IDX13_VEC8(); return minidx;
            case 13: MIN_IDX_REDUCE_IDX14_VEC8(); return minidx;
            case 14: MIN_IDX_REDUCE_IDX15_VEC8(); return minidx;
            case 15: MIN_IDX_REDUCE_IDX16_VEC8(); return minidx;
            case 16: MIN_IDX_REDUCE_IDX17_VEC8(); return minidx;
            case 17: MIN_IDX_REDUCE_IDX18_VEC8(); return minidx;
            case 18: MIN_IDX_REDUCE_IDX19_VEC8(); return minidx;
            case 19: MIN_IDX_REDUCE_IDX20_VEC8(); return minidx;
            case 20: MIN_IDX_REDUCE_IDX21_VEC8(); return minidx;
            case 21: MIN_IDX_REDUCE_IDX22_VEC8(); return minidx;
            case 22: MIN_IDX_REDUCE_IDX23_VEC8(); return minidx;
            case 23: MIN_IDX_REDUCE_IDX24_VEC8(); return minidx;
                /* clang-format on */
            }
        } while (0);

        rb_raise(
          rb_eException, "invalid DHeap min_child size: %zd", last - idx + 1);
    }
}

#    undef MINIDX2
#    undef MINIDX3

#else

static inline size_t
min_index_loop(ENTRY entries[], size_t idx0, size_t last)
{
    for (size_t idx = idx0 + 1; idx <= last; idx++) {
        if (entries[idx].score < entries[idx0].score) {
            idx0 = idx;
        }
    }
    return idx0;
}

#endif

static inline size_t
dheap_min_child(dheap_t *heap, const size_t parent, const size_t last_index)
{
    const size_t idx0 = DHEAP_IDX_CHILD_0(heap, parent);
    const size_t idxd = DHEAP_IDX_CHILD_D(heap, parent);
    const size_t last = (LIKELY(idxd <= last_index)) ? idxd : last_index;

#ifdef DHEAP_SIMD_ENABLED
    return min_index_simd(heap, idx0, last);
#else
    return min_index_loop(heap->entries, idx0, last);
#endif
}

#define DHEAP_CAN_SIFT_DOWN(heap, index, last_index)                           \
    (LIKELY(1 <= last_index && index <= DHEAP_IDX_PARENT(heap, last_index)))

#define DHEAP_SIFT_DOWN(T, heap, i)                                            \
    do {                                                                       \
        size_t sift_idx = i;                                                   \
        size_t last_idx = DHEAP_IDX_LAST(heap);                                \
        ASSERT_DHEAP_IDX_OK(heap, sift_idx);                                   \
        if (DHEAP_CAN_SIFT_DOWN(heap, sift_idx, last_idx)) {                   \
            ENTRY  entry       = heap->entries[sift_idx];                      \
            size_t last_parent = DHEAP_IDX_PARENT(heap, last_idx);             \
            while (sift_idx <= last_parent) {                                  \
                size_t min_child = dheap_min_child(heap, sift_idx, last_idx);  \
                if (CMP_LTE(entry.score, DHEAP_SCORE(heap, min_child))) break; \
                DHEAP_SET(T, heap, sift_idx, (heap)->entries[min_child]);      \
                sift_idx = min_child;                                          \
            }                                                                  \
            DHEAP_SET(T, heap, sift_idx, entry);                               \
        }                                                                      \
    } while (0)

/********************************************************************
 *
 * DHeap attributes
 *
 ********************************************************************/

/*
 * @return [Integer] the number of elements in the heap
 */
static VALUE
dheap_size(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    return ULONG2NUM(heap->size);
}

#define DHEAP_EMPTY_P(heap) UNLIKELY((heap)->size <= 0)

/*
 * @return [Boolean] if the heap is empty
 */
static VALUE
dheap_empty_p(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    return DHEAP_EMPTY_P(heap) ? Qtrue : Qfalse;
}

/*
 * @return [Integer] the maximum number of children per parent
 */
static VALUE
dheap_attr_d(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    return INT2FIX(heap->d);
}

/********************************************************************
 *
 * DHeap push
 *
 ********************************************************************/

#define DHEAP_PUSH(T, heap, entry)                                             \
    do {                                                                       \
        dheap_ensure_room_for_push(heap, 1);                                   \
        DHEAP_SET(T, heap, (heap)->size, *(entry));                            \
        ++heap->size;                                                          \
        DHEAP_SIFT_UP(T, heap, DHEAP_IDX_LAST(heap));                          \
    } while (0)

static inline void
dheap_push_entry(VALUE self, ENTRY *entry)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    DHEAP_PUSH(dheap, heap, entry);
}

#ifdef DHEAP_MAP
static inline void
dheapmap_update_entry(dheap_t *heap, size_t index, ENTRY *entry)
{
    SCORE prev = DHEAP_SCORE(heap, index);
    DHEAP_SET(dheapmap, heap, index, *entry);
    if (CMP_LT(prev, entry->score)) {
        DHEAP_SIFT_DOWN(dheapmap, heap, index);
    } else {
        DHEAP_SIFT_UP(dheapmap, heap, index);
    }
}

static inline void
dheapmap_push_entry(VALUE self, ENTRY *entry)
{
    dheap_t *heap   = get_dheap_struct_unfrozen(self);
    VALUE    idxval = rb_hash_lookup2(heap->indexes, entry->value, Qfalse);
    if (idxval) {
        size_t index = NUM2ULONG(idxval);
        dheapmap_update_entry(heap, index, entry);
        return;
    }
    DHEAP_PUSH(dheapmap, heap, entry);
}
#endif

/*
 * Inserts a value into the heap, using a score to determine sort-order.
 *
 * Score comes first, as an analogy with the +Array#insert+ index.
 *
 * Time complexity: <b>O(log n / log d)</b> <i>(worst-case)</i>
 *
 * @param score [Integer,Float,#to_f] a score to compare against other
 * scores.
 * @param value [Object] an object that is associated with the score.
 *
 * @return [self]
 */
static VALUE
dheap_insert(VALUE self, VALUE score, VALUE value)
{
    ENTRY entry = { VAL2SCORE(score), value };
    dheap_push_entry(self, &entry);
    return self;
}

#ifdef DHEAP_MAP
/* (see DHeap#insert) */
static VALUE
dheapmap_insert(VALUE self, VALUE score, VALUE value)
{
    ENTRY entry = { VAL2SCORE(score), value };
    dheapmap_push_entry(self, &entry);
    return self;
}
#endif

static inline ENTRY
dheap_push_args_to_entry(int argc, VALUE *argv)
{
    ENTRY entry;
    rb_check_arity(argc, 1, 2);
    entry.value = argv[0];
    entry.score = VAL2SCORE(argc < 2 ? entry.value : argv[1]);
    return entry;
}

/*
 * @overload push(value, score = value)
 *
 * Push a value onto heap, using a score to determine sort-order.
 *
 * Value comes first because the separate score is optional, and because it
 * feels like a more natural variation on +Array#push+ or +Queue#enq+.  If a
 * score isn't provided, the value must be an Integer or can be cast with
 * +Float(value)+.
 *
 * Time complexity: <b>O(log n / log d)</b> <i>(worst-case)</i>
 *
 * @param value [Object] an object that is associated with the score.
 * @param score [Integer,Float,#to_f] a score to compare against other
 * scores.
 *
 * @return [self]
 */
static VALUE
dheap_push(int argc, VALUE *argv, VALUE self)
{
    ENTRY entry = dheap_push_args_to_entry(argc, argv);
    dheap_push_entry(self, &entry);
    return self;
}

#ifdef DHEAP_MAP
/* (see DHeap#push) */
static VALUE
dheapmap_push(int argc, VALUE *argv, VALUE self)
{
    ENTRY entry = dheap_push_args_to_entry(argc, argv);
    dheapmap_push_entry(self, &entry);
    return self;
}
#endif

/*
 * Pushes a value onto the heap.
 *
 * The score will be derived from the value, by using the value itself if it
 * is an Integer, otherwise by casting it with +Float(value)+.
 *
 * Time complexity: <b>O(log n / log d)</b> <i>(worst-case)</i>
 *
 * @param value [Integer,#to_f] a value with an intrinsic numeric score
 * @return [self]
 */
static VALUE
dheap_lshift(VALUE self, VALUE value)
{
    ENTRY entry = { VAL2SCORE(value), value };
    dheap_push_entry(self, &entry);
    return self;
}

#ifdef DHEAP_MAP
/* (see DHeap#<<) */
static VALUE
dheapmap_lshift(VALUE self, VALUE value)
{
    ENTRY entry = { VAL2SCORE(value), value };
    dheapmap_push_entry(self, &entry);
    return self;
}
#endif

/********************************************************************
 *
 * DHeap pop and peek
 *
 ********************************************************************/

#define PEEK_VALUE(heap)            DHEAP_VALUE(heap, 0)
#define PEEK_SCORE(heap)            DHEAP_SCORE(heap, 0)
#define PEEK_WITH_SCORE(heap)       DHEAP_ENTRY_ARY(heap, 0)
#define PEEK_LT_P(heap, max_score)  _PEEK_CMP_P(heap, CMP_LT, max_score)
#define PEEK_LTE_P(heap, max_score) _PEEK_CMP_P(heap, CMP_LTE, max_score)
#define _PEEK_CMP_P(heap, cmp, score)                                          \
    ((!DHEAP_EMPTY_P(heap) && cmp(PEEK_SCORE(heap), (score)))                  \
       ? PEEK_VALUE(heap)                                                      \
       : 0)

/*
 * Returns the next value on the heap, and its score, without popping it
 *
 * Time complexity: <b>O(1)</b> <i>(worst-case)</i>
 * @return [nil,Array<(Object, Numeric)>] the next value and its score
 *
 * @see #peek
 * @see #peek_score
 */
static VALUE
dheap_peek_with_score(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    return PEEK_WITH_SCORE(heap);
}

/*
 * Returns the next score on the heap, without the value and without popping
 * it.
 *
 * Time complexity: <b>O(1)</b> <i>(worst-case)</i>
 * @return [nil, Numeric] the next score, if there is one
 *
 * @see #peek
 * @see #peek_with_score
 */
static VALUE
dheap_peek_score(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    if (DHEAP_EMPTY_P(heap)) return Qnil;
    return SCORE2NUM(PEEK_SCORE(heap));
}

/*
 * Returns the next value on the heap to be popped without popping it.
 *
 * Time complexity: <b>O(1)</b> <i>(worst-case)</i>
 * @return [nil, Object] the next value to be popped without popping it.
 *
 * @see #peek_score
 * @see #peek_with_score
 */
static VALUE
dheap_peek(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    if (DHEAP_EMPTY_P(heap)) return Qnil;
    return PEEK_VALUE(heap);
}

#define DHEAP_POP_COMMON_INIT(self)                                            \
    dheap_t *heap = get_dheap_struct_unfrozen(self);                           \
    if (DHEAP_EMPTY_P(heap)) return Qnil;

#define DHEAP_DELETE_0(T, heap)                                                \
    do {                                                                       \
        _DELETE_ENTRY(T, heap, 0);                                             \
        if (0 < --(heap)->size) {                                              \
            DHEAP_SET(T, (heap), 0, (heap)->entries[(heap)->size]);            \
            DHEAP_SIFT_DOWN(T, (heap), 0);                                     \
        }                                                                      \
    } while (0)

#define _DELETE_ENTRY(T, heap, idx)    _DELETE_ENTRY_##T(heap, idx)
#define _DELETE_ENTRY_dheap(heap, idx) /* noop */
#define _DELETE_ENTRY_dheapmap(heap, idx)                                      \
    rb_hash_delete(heap->indexes, DHEAP_VALUE(heap, idx))

#define POP(T, heap, popped)            _POP(T, VALUE, heap, popped)
#define POP_WITH_SCORE(T, heap, popped) _POP(T, WITH_SCORE, heap, popped)
#define POP_LT(T, heap, max, popped)    _POP_CMP(T, heap, LT, max, popped)
#define POP_LTE(T, heap, max, popped)   _POP_CMP(T, heap, LTE, max, popped)

#define _POP_CMP(T, heap, cmp, cmp_score, popped)                              \
    do {                                                                       \
        if ((*(popped) = PEEK_##cmp##_P(heap, cmp_score))) {                   \
            DHEAP_DELETE_0(T, heap);                                           \
        } else {                                                               \
            *(popped) = Qnil;                                                  \
        }                                                                      \
    } while (0)

#define _POP(T, peek_type, heap, popped)                                       \
    do {                                                                       \
        if (DHEAP_EMPTY_P(heap)) {                                             \
            *(popped) = Qnil;                                                  \
        } else {                                                               \
            *(popped) = PEEK_##peek_type(heap);                                \
            DHEAP_DELETE_0(T, heap);                                           \
        }                                                                      \
    } while (0)

/*
 * Pops the minimum value from the top of the heap
 *
 * Time complexity: <b>O(d log n / log d)</b> <i>(worst-case)</i>
 *
 * @return [Object] the value with the minimum score
 *
 * @see #peek
 * @see #pop_lt
 * @see #pop_lte
 * @see #pop_with_score
 */
static VALUE
dheap_pop(VALUE self)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP(dheap, heap, &popped);
    return popped;
}

#ifdef DHEAP_MAP
/* (see DHeap#pop) */
static VALUE
dheapmap_pop(VALUE self)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP(dheapmap, heap, &popped);
    return popped;
}
#endif

/*
 * Pops the minimum value from the top of the heap, along with its score.
 *
 * Time complexity: <b>O(d log n / log d)</b> <i>(worst-case)</i>
 *
 * @return [nil,Array<(Object, Numeric)>] the next value and its score
 *
 * @see #pop
 * @see #peek_with_score
 */
static VALUE
dheap_pop_with_score(VALUE self)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP_WITH_SCORE(dheap, heap, &popped);
    return popped;
}

#ifdef DHEAP_MAP
/* (see DHeap#pop_with_score) */
static VALUE
dheapmap_pop_with_score(VALUE self)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP_WITH_SCORE(dheapmap, heap, &popped);
    return popped;
}
#endif

#define DHEAP_POP_IF(heap, cmp, max_score)                                     \
    if (!cmp(PEEK_SCORE(heap), VAL2SCORE(max_score))) return Qnil

/*
 * Pops the minimum value only if it is less than or equal to a max score.
 *
 * @param max_score [Integer,#to_f] the maximum score to be popped
 *
 * Time complexity: <b>O(d log n / log d)</b> <i>(worst-case)</i>
 *
 * @return [Object] the value with the minimum score
 *
 * @see #peek
 * @see #pop
 * @see #pop_lt
 * @see #pop_all_below
 */
static VALUE
dheap_pop_lte(VALUE self, VALUE max_score)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP_LTE(dheap, heap, VAL2SCORE(max_score), &popped);
    return popped;
}

#ifdef DHEAP_MAP
/* (see DHeap#pop_lte) */
static VALUE
dheapmap_pop_lte(VALUE self, VALUE max_score)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP_LTE(dheapmap, heap, VAL2SCORE(max_score), &popped);
    return popped;
}
#endif

/*
 * Pops the minimum value only if it is less than a max score.
 *
 * @param max_score [Integer,#to_f] the maximum score to be popped
 *
 * Time complexity: <b>O(d log n / log d)</b> <i>(worst-case)</i>
 *
 * @return [Object] the value with the minimum score
 *
 * @see #peek
 * @see #pop
 * @see #pop_lte
 * @see #pop_all_below
 */
static VALUE
dheap_pop_lt(VALUE self, VALUE max_score)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP_LT(dheap, heap, VAL2SCORE(max_score), &popped);
    return popped;
}

#ifdef DHEAP_MAP
/* (see DHeap#pop_lt) */
static VALUE
dheapmap_pop_lt(VALUE self, VALUE max_score)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    VALUE    popped;
    POP_LT(dheapmap, heap, VAL2SCORE(max_score), &popped);
    return popped;
}
#endif

#define POP_ALL_BELOW(T, heap, max_score, array)                               \
    do {                                                                       \
        VALUE val = Qnil;                                                      \
        if (RB_TYPE_P(array, T_ARRAY)) {                                       \
            while ((val = PEEK_LT_P(heap, max_score))) {                       \
                DHEAP_DELETE_0(T, heap);                                       \
                rb_ary_push(array, val);                                       \
            }                                                                  \
        } else {                                                               \
            while ((val = PEEK_LT_P(heap, max_score))) {                       \
                DHEAP_DELETE_0(T, heap);                                       \
                rb_funcall(array, id_lshift, 1, val);                          \
            }                                                                  \
        }                                                                      \
    } while (0)

/*
 * @overload pop_all_below(max_score, receiver = [])
 *
 * Pops all value with score less than max score.
 *
 * Time complexity: <b>O(m * d log n / log d)</b>, <i>m = number popped</i>
 *
 * @param max_score [Integer,#to_f] the maximum score to be popped
 * @param receiver  [Array,#<<] object onto which the values will be pushed,
 *                              in order by score.
 *
 * @return [Object] the object onto which the values were pushed
 *
 * @see #pop
 * @see #pop_lt
 */
static VALUE
dheap_pop_all_below(int argc, VALUE *argv, VALUE self)
{
    dheap_t *heap      = get_dheap_struct_unfrozen(self);
    SCORE    max_score = (argc) ? VAL2SCORE(argv[0]) : 0.0;
    VALUE    array     = (argc == 1) ? rb_ary_new() : argv[1];
    rb_check_arity(argc, 1, 2);
    DHEAP_DISPATCH_STMT(heap, POP_ALL_BELOW, max_score, array);
    return array;
}

/********************************************************************
 *
 * DHeap, misc methods
 *
 ********************************************************************/

static VALUE
dheap_to_a(VALUE self)
{
    dheap_t *heap  = get_dheap_struct(self);
    VALUE    array = rb_ary_new_capa(heap->size);
    for (size_t i = 0; i < heap->size; i++) {
        rb_ary_push(array, DHEAP_ENTRY_ARY(heap, i));
    }
    return array;
}

/*
 * Clears all values from the heap, leaving it empty.
 *
 * @return [self]
 */
static VALUE
dheap_clear(VALUE self)
{
    dheap_t *heap = get_dheap_struct_unfrozen(self);
    if (!DHEAP_EMPTY_P(heap)) {
        heap->size = 0;
#ifdef DHEAP_MAP
        if (DHEAPMAP_P(heap)) rb_hash_clear(heap->indexes);
#endif
    }
    return self;
}

/********************************************************************
 *
 * DHeap::Map methods
 *
 ********************************************************************/

#ifdef DHEAP_MAP

/*
 * Retrieves the score that has been assigned to a heap member.
 *
 * Time complexity: <b>O(1)</b>
 *
 * @param object [Object] an object to lookup
 * @return [Float,Integer,nil] the score associated with the object,
 *                             or nil if the object isn't a member
 */
static VALUE
dheapmap_aref(VALUE self, VALUE object)
{
    dheap_t *heap   = get_dheap_struct(self);
    VALUE    idxval = rb_hash_lookup2(heap->indexes, object, Qfalse);
    if (idxval) {
        size_t index = NUM2ULONG(idxval);
        return SCORE2NUM(DHEAP_SCORE(heap, index));
    }
    return Qnil;
}

/*
 * Assign a score to an object, adding it to the heap or updating as
 * necessary.
 *
 * Time complexity: <b>O(log n)</b> (score decrease will be faster than
 * score increase)
 *
 * @param object [Object] an object to lookup
 * @param score [Integer,#to_f] the score to set
 * @return [Float,Integer] the score
 */
static VALUE
dheapmap_aset(VALUE self, VALUE object, VALUE score)
{
    dheapmap_insert(self, score, object);
    return score;
}

#endif

/********************************************************************
 *
 * DHeap setup
 *
 ********************************************************************/

void
Init_d_heap(void)
{
#ifdef DHEAP_ENABLE_MIN_IDX_AVX512
    idx64x8 = _mm512_setr_epi64(0L, 1L, 2L, 3L, 4L, 5L, 6L, 7L);
    incrby8 = _mm512_set1_epi64(8L);
#endif
#ifdef DHEAP_ENABLE_MIN_IDX_AVX2
    idx64x4 = _mm256_setr_epi64x(0L, 1L, 2L, 3L);
    incrby4 = _mm256_set1_epi64x(4L);
#endif
#ifdef DHEAP_ENABLE_MIN_IDX_SSE2
    incrby2 = _mm_set1_epi64x(2L);
#endif

    VALUE rb_cDHeap = rb_define_class("DHeap", rb_cObject);
#ifdef DHEAP_MAP
    VALUE rb_cDHeapMap = rb_define_class_under(rb_cDHeap, "Map", rb_cDHeap);
#endif

    id_cmp    = rb_intern_const("<=>");
    id_abs    = rb_intern_const("abs");
    id_lshift = rb_intern_const("<<");
    id_uminus = rb_intern_const("-@");

    rb_define_alloc_func(rb_cDHeap, dheap_s_alloc);

#ifdef DHEAP_MAP
#    define def_override_inherited(rb_name, c_name, argc)                      \
        rb_define_method(rb_cDHeap, rb_name, dheap_##c_name, argc);            \
        rb_define_method(rb_cDHeapMap, rb_name, dheapmap_##c_name, argc);
#else
#    define def_override_inherited(rb_name, c_name, argc)                      \
        rb_define_method(rb_cDHeap, rb_name, dheap_##c_name, argc);
#endif

    /*
     * This is based on INT_MAX. But it is very very unlikely you will want
     * a large value for d.  The tradeoff is that higher d values give
     * faster push and slower pop.  If you expect pushes and pops to be
     * balanced, then just stick with the default.  If you expect more
     * pushes than pops, it might be worthwhile to increase d.
     */
    rb_define_const(rb_cDHeap, "MAX_D", INT2NUM(DHEAP_MAX_D));

    /*
     * d=4 uses the fewest comparisons for (worst case) insert + delete-min.
     */
    rb_define_const(rb_cDHeap, "DEFAULT_D", INT2NUM(DHEAP_DEFAULT_D));

    /*
     * The default heap capacity.  The heap grows automatically as
     * necessary, so you shouldn't need to worry about this.
     */
    rb_define_const(rb_cDHeap, "DEFAULT_CAPA", INT2NUM(DHEAP_DEFAULT_CAPA));

    rb_define_private_method(rb_cDHeap, "__init_without_kw__", dheap_init, 3);
    rb_define_method(rb_cDHeap, "initialize_copy", dheap_initialize_copy, 1);

    rb_define_method(rb_cDHeap, "d", dheap_attr_d, 0);
    rb_define_method(rb_cDHeap, "size", dheap_size, 0);
    rb_define_method(rb_cDHeap, "empty?", dheap_empty_p, 0);
    rb_define_method(rb_cDHeap, "to_a", dheap_to_a, 0);

    rb_define_method(rb_cDHeap, "clear", dheap_clear, 0);
    rb_define_method(rb_cDHeap, "peek", dheap_peek, 0);
    rb_define_method(rb_cDHeap, "peek_score", dheap_peek_score, 0);
    rb_define_method(rb_cDHeap, "peek_with_score", dheap_peek_with_score, 0);
    rb_define_method(rb_cDHeap, "pop_all_below", dheap_pop_all_below, -1);

    def_override_inherited("insert", insert, 2);
    def_override_inherited("push", push, -1);
    def_override_inherited("<<", lshift, 1);

    def_override_inherited("pop", pop, 0);
    def_override_inherited("pop_lt", pop_lt, 1);
    def_override_inherited("pop_lte", pop_lte, 1);
    def_override_inherited("pop_with_score", pop_with_score, 0);

#ifdef DHEAP_MAP
    rb_define_method(rb_cDHeapMap, "[]", dheapmap_aref, 1);
    rb_define_method(rb_cDHeapMap, "[]=", dheapmap_aset, 2);
#endif
}
