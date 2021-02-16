#include "ruby.h"
#include <float.h>
#include <math.h>

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

#define DHEAP_DEFAULT_D 6
#define DHEAP_MAX_D     INT_MAX

// sizeof(ENTRY) => 16 bytes, 128-bits
// one kilobyte = 32 * 32 bytes
#define DHEAP_DEFAULT_CAPA  32
#define DHEAP_MAX_CAPA      (SIZE_MAX / (int)sizeof(ENTRY))
#define DHEAP_CAPA_INCR_MAX (10 * 1024 * 1024 / (int)sizeof(ENTRY))

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

#ifdef DEBUG
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

static inline size_t
dheap_min_child(dheap_t *heap, size_t parent, size_t last_index)
{
    size_t min_child = DHEAP_IDX_CHILD_0(heap, parent);
    size_t last_sib  = DHEAP_IDX_CHILD_D(heap, parent);
    if (UNLIKELY(last_index < last_sib)) last_sib = last_index;

    for (size_t sibidx = min_child + 1; sibidx <= last_sib; ++sibidx) {
        if (CMP_LT(DHEAP_SCORE(heap, sibidx), DHEAP_SCORE(heap, min_child))) {
            min_child = sibidx;
        }
    }
    return min_child;
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
 * @param score [Integer,Float,#to_f] a score to compare against other scores.
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
 * @param score [Integer,Float,#to_f] a score to compare against other scores.
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
 * The score will be derived from the value, by using the value itself if it is
 * an Integer, otherwise by casting it with +Float(value)+.
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
 * Assign a score to an object, adding it to the heap or updating as necessary.
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
     * This is based on INT_MAX. But it is very very unlikely you will want a
     * large value for d.  The tradeoff is that higher d values give faster push
     * and slower pop.  If you expect pushes and pops to be balanced, then just
     * stick with the default.  If you expect more pushes than pops, it might be
     * worthwhile to increase d.
     */
    rb_define_const(rb_cDHeap, "MAX_D", INT2NUM(DHEAP_MAX_D));

    /*
     * d=4 uses the fewest comparisons for (worst case) insert + delete-min.
     */
    rb_define_const(rb_cDHeap, "DEFAULT_D", INT2NUM(DHEAP_DEFAULT_D));

    /*
     * The default heap capacity.  The heap grows automatically as necessary, so
     * you shouldn't need to worry about this.
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
