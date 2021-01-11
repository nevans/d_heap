#include <float.h>
#include "d_heap.h"

#define SCORE_AS_LONG_DOUBLE 1

ID id_cmp; // <=>
ID id_abs; // abs

#ifdef SCORE_AS_LONG_DOUBLE
    #define SCORE long double
#else
    #define SCORE VALUE
#endif

typedef struct dheap_struct {
    int d;
    VALUE values;
#ifdef SCORE_AS_LONG_DOUBLE
    long size;
    long capa;
    SCORE *cscores;
#else
    VALUE scores;  // T_ARRAY of comparable objects
#endif
} dheap_t;

#define DHEAP_VALUE(heap, idx) RARRAY_AREF((heap)->values, idx)
#define DHEAP_IDX_LAST(heap) (DHEAP_SIZE((heap)) - 1)
#define DHEAP_IDX_PARENT(heap, idx) (((idx) - 1) / (heap)->d)
#define DHEAP_IDX_CHILD0(heap, idx) (((idx) * (heap)->d) + 1)

#ifdef SCORE_AS_LONG_DOUBLE
    #define DHEAP_SIZE(heap) ((heap)->size)
#else
    #define DHEAP_SIZE(heap) (RARRAY_LEN((heap)->scores))
#endif

#ifdef SCORE_AS_LONG_DOUBLE
    #define DHEAP_SCORE(heap, idx)                                       \
       (idx < 0 || heap->size <= idx ? (SCORE)0 :                        \
        ((heap)->cscores[idx]))
#else
    #define DHEAP_SCORE(heap, idx) RARRAY_AREF((heap)->scores, idx)
#endif

#ifdef SCORE_AS_LONG_DOUBLE

#define CMP_LT(a, b)  (a <  b)
#define CMP_LTE(a, b) (a <= b)
#define CMP_GT(a, b)  (a >  b)
#define CMP_GTE(a, b) (a >= b)

#if LDBL_MANT_DIG < SIZEOF_UNSIGNED_LONG_LONG * 8
#error 'unsigned long long' should fit into 'long double' mantissa
#endif

// copied and modified from ruby's object.c
#define FIX2SCORE(x) (long double)FIX2LONG(x)
// We could translate a much wider range of values to long double by
// implementing a new `rb_big2ldbl(x)` function. But requires reaching into
// T_BIGNUM internals.
static inline long double
BIG2SCORE(VALUE x)
{
    if (RBIGNUM_POSITIVE_P(x)) {
        unsigned long long ull = rb_big2ull(x);
        return (long double)ull;
    } else {
        unsigned long long ull;
        long double ldbl;
        x = rb_funcall(x, id_abs, 0);
        ull  = rb_big2ull(x);
        ldbl = (long double) ull;
        return -ldbl;
    }
}
#define INT2SCORE(x) \
    (FIXNUM_P(x) ? FIX2SCORE(x) : BIG2SCORE(x))
#define NUM2SCORE(x)                                                     \
    (FIXNUM_P(x) ? FIX2SCORE(x) :                                        \
     RB_TYPE_P(x, T_BIGNUM) ? BIG2SCORE(x) :                             \
     (Check_Type(x, T_FLOAT), (long double)RFLOAT_VALUE(x)))
static inline long double
RAT2SCORE(VALUE x)
{
    VALUE num = rb_rational_num(x);
    VALUE den = rb_rational_den(x);
    return NUM2SCORE(num) / NUM2SCORE(den);
}

/*
 * Convert both T_FIXNUM and T_FLOAT (and sometimes T_BIGNUM, T_RATIONAL,
 * String, etc) to SCORE
 * * with no loss of precision (where possible for Integer and Float),
 * * raises an exception if
 *   * a positive integer is too large for unsigned long long (should be 64bit)
 *   * a negative integer is too small for   signed long long (should be 64bit)
 * * reduced to long double (should be 80 or 128 bit) if it is Rational
 * * reduced to double precision if the value is convertable by Float(x)
 */
static inline long double
VAL2SCORE(VALUE score)
{
    // assert that long double can hold 'unsigned long long':
    // static_assert(sizeof(unsigned long long) * 8 <= LDBL_MANT_DIG);
    // assert that long double can hold T_FLOAT
    // static_assert(sizeof(double) <= sizeof(long double));

    switch (TYPE(score)) {
        case T_FIXNUM:
            return FIX2SCORE(score);
        case T_BIGNUM:
            return BIG2SCORE(score);
        case T_RATIONAL:
            return RAT2SCORE(score);
        default:
            return (long double)(NUM2DBL(rb_Float(score)));
    }
}

#else

#define VAL2SCORE(score) (score)

#define CMP_LT(a, b)  (optimized_cmp(a, b) <  0)
#define CMP_LTE(a, b) (optimized_cmp(a, b) <= 0)
#define CMP_GT(a, b)  (optimized_cmp(a, b) >  0)
#define CMP_GTE(a, b) (optimized_cmp(a, b) >= 0)

// from internal/compar.h
#define STRING_P(s) (RB_TYPE_P((s), T_STRING) && CLASS_OF(s) == rb_cString)
/*
 * short-circuit evaluation for a few basic types.
 *
 * Only Integer, Float, and String are optimized,
 * and only when both arguments are the same type.
 */
static inline int
optimized_cmp(SCORE a, SCORE b) {
    if (a == b) // Fixnum equality and object equality
        return 0;
    if (FIXNUM_P(a) && FIXNUM_P(b))
        return (FIX2LONG(a) < FIX2LONG(b)) ? -1 : 1;
    if (RB_FLOAT_TYPE_P(a) && RB_FLOAT_TYPE_P(b))
    {
        double x, y;
        x = RFLOAT_VALUE(a);
        y = RFLOAT_VALUE(b);
        if (isnan(x) || isnan(y)) rb_cmperr(a, b); // raise ArgumentError
        return (x < y) ? -1 : ((x == y) ? 0 : 1);
    }
    if (RB_TYPE_P(a, T_BIGNUM) && RB_TYPE_P(b, T_BIGNUM))
        return FIX2INT(rb_big_cmp(a, b));
    if (STRING_P(a) && STRING_P(b))
        return rb_str_cmp(a, b);

    // give up on an optimized version and just call (a <=> b)
    return rb_cmpint(rb_funcallv(a, id_cmp, 1, &b), a, b);
}

#endif

#define DHEAP_Check_d_size(d) do {                            \
    if (d < 2) {                                              \
        rb_raise(rb_eArgError, "DHeap d=%d is too small", d); \
    }                                                         \
    if (d > DHEAP_MAX_D) {                                    \
        rb_raise(rb_eArgError, "DHeap d=%d is too large", d); \
    }                                                         \
} while (0)

#define DHEAP_Check_Index(index, last_index) do {                     \
    if (index < 0) {                                                  \
        rb_raise(rb_eIndexError, "DHeap index %ld too small", index); \
    }                                                                 \
    else if (last_index < index) {                                    \
        rb_raise(rb_eIndexError, "DHeap index %ld too large", index); \
    }                                                                 \
} while (0)

static void
dheap_compact(void *ptr)
{
    dheap_t *heap = ptr;
#ifndef SCORE_AS_LONG_DOUBLE
    if (heap->scores) dheap_gc_location( heap->scores );
#endif
    if (heap->values) dheap_gc_location( heap->values );
}

static void
dheap_mark(void *ptr)
{
    dheap_t *heap = ptr;
#ifndef SCORE_AS_LONG_DOUBLE
    if (heap->scores) rb_gc_mark_movable(heap->scores);
#endif
    if (heap->values) rb_gc_mark_movable(heap->values);
}

static void
dheap_free(void *ptr)
{
#ifdef SCORE_AS_LONG_DOUBLE
    dheap_t *heap = ptr;
    heap->size = 0;
    if (heap->cscores) {
        ruby_xfree(heap->cscores);
        heap->cscores = NULL;
    }
    heap->capa = 0;
#endif
    xfree(ptr);
}

static size_t
dheap_memsize(const void *ptr)
{
    const dheap_t *heap = ptr;
    size_t size = 0;
    size += sizeof(*heap);
#ifdef SCORE_AS_LONG_DOUBLE
    size += sizeof(long double) * heap->capa;
#endif
    return size;
}

static const rb_data_type_t dheap_data_type = {
    "DHeap",
    {
        (void (*)(void*))dheap_mark,
        (void (*)(void*))dheap_free,
        (size_t (*)(const void *))dheap_memsize,
        dheap_compact_callback(dheap_compact),
    },
    0, 0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

static VALUE
dheap_s_alloc(VALUE klass)
{
    VALUE obj;
    dheap_t *heap;

    obj = TypedData_Make_Struct(klass, dheap_t, &dheap_data_type, heap);
    heap->d = DHEAP_DEFAULT_D;
    heap->values = Qnil;

#ifdef SCORE_AS_LONG_DOUBLE
    heap->size = 0;
    heap->capa = 0;
    heap->cscores = NULL;
#else
    heap->scores = Qnil;
#endif

    return obj;
}

static inline dheap_t *
get_dheap_struct(VALUE self)
{
    dheap_t *heap;
    TypedData_Get_Struct(self, dheap_t, &dheap_data_type, heap);
    Check_Type(heap->values, T_ARRAY); // ensure it's been initialized
    return heap;
}

#ifdef SCORE_AS_LONG_DOUBLE

static void
dheap_set_capa(dheap_t *heap, long new_capa)
{
    long double *new, *old;
    // Do nothing if we already have the capacity or are resizing too small
    if (new_capa <= heap->capa) return;
    if (new_capa <= heap->size) return;

    // allocate
    new = ruby_xcalloc(new_capa, sizeof(long double));
    old = heap->cscores;

    // copy contents
    if (old) {
        MEMCPY(new, old, long double, heap->size);
        ruby_xfree(old);
    }

    // set vars
    heap->cscores = new;
    heap->capa = new_capa;
}

static void
dheap_ensure_room_for_push(dheap_t *heap, long incr_by)
{
    long new_size = heap->size + incr_by;

    // check for overflow of new_size
    if (DHEAP_MAX_SIZE - incr_by < heap->size)
        rb_raise(rb_eIndexError, "index %ld too big", new_size);

    // if it existing capacity is too small
    if (heap->capa < new_size) {
        // double it...
        long new_capa = new_size * 2;
        if (DHEAP_CAPA_INCR_MAX < new_size)
            new_size = new_size + DHEAP_CAPA_INCR_MAX;
        // check for overflow of new_capa
        if (DHEAP_MAX_SIZE / 2 < new_size) new_capa = DHEAP_MAX_SIZE;
        // cap max incr_by
        if (heap->capa + DHEAP_CAPA_INCR_MAX < new_capa)
            new_capa = heap->capa + DHEAP_CAPA_INCR_MAX;

        dheap_set_capa(heap, new_capa);
    }
}

#endif

/*
 * @overload initialize(d = DHeap::DEFAULT_D)
 *   Initialize a _d_-ary min-heap.
 *
 *   @param d [Integer] maximum number of children per parent
 */
static VALUE
dheap_initialize(int argc, VALUE *argv, VALUE self) {
    dheap_t *heap;
    int d;

    rb_check_arity(argc, 0, 1);
    TypedData_Get_Struct(self, dheap_t, &dheap_data_type, heap);

    d = argc ? NUM2INT(argv[0]) : DHEAP_DEFAULT_D;
    DHEAP_Check_d_size(d);
    heap->d = d;

    heap->values = rb_ary_new_capa(DHEAP_DEFAULT_SIZE);

#ifdef SCORE_AS_LONG_DOUBLE
    dheap_set_capa(heap, DHEAP_DEFAULT_SIZE);
#else
    heap->scores = rb_ary_new_capa(DHEAP_DEFAULT_SIZE);
#endif

    return self;
}

/* :nodoc: */
static VALUE
dheap_initialize_copy(VALUE copy, VALUE orig)
{
    dheap_t *heap_copy;
    dheap_t *heap_orig = get_dheap_struct(orig);

    rb_check_frozen(copy);
    TypedData_Get_Struct(copy, dheap_t, &dheap_data_type, heap_copy);

    heap_copy->d = heap_orig->d;

    heap_copy->values = rb_ary_new();
    rb_ary_replace(heap_copy->values, heap_orig->values);

#ifdef SCORE_AS_LONG_DOUBLE
    dheap_set_capa(heap_copy, heap_orig->capa);
    heap_copy->size = heap_orig->size;
    if (heap_copy->size)
        MEMCPY(heap_orig->cscores, heap_copy->cscores, long double, heap_orig->size);
#else
    heap_copy->scores = rb_ary_new();
    rb_ary_replace(heap_copy->scores, heap_orig->scores);
#endif

    return copy;
}

static inline void
dheap_assign(dheap_t *heap, long idx, SCORE score, VALUE value)
{
#ifdef SCORE_AS_LONG_DOUBLE
    heap->cscores[idx] = score;
    rb_ary_store(heap->values, idx, value);
#else
    rb_ary_store(heap->scores, idx, score);
    rb_ary_store(heap->values, idx, value);
#endif
}

VALUE
dheap_ary_sift_up(dheap_t *heap, long sift_index) {
    VALUE sift_value;
    SCORE sift_score;

    long last_index = DHEAP_IDX_LAST(heap);
    DHEAP_Check_Index(sift_index, last_index);

    sift_value = DHEAP_VALUE(heap, sift_index);
    sift_score = DHEAP_SCORE(heap, sift_index);

    // sift it up to where it belongs
    for (long parent_index; 0 < sift_index; sift_index = parent_index) {
        SCORE parent_score;
        VALUE parent_value;

        debug(rb_sprintf("sift up(%"PRIsVALUE", %d, %ld)", heap->values, heap->d, sift_index));
        parent_index = DHEAP_IDX_PARENT(heap, sift_index);
        parent_score = DHEAP_SCORE(heap, parent_index);

        // parent is smaller: heap is restored
        if (CMP_LTE(parent_score, sift_score)) break;

        // parent is larger: swap and continue sifting up
        parent_value = DHEAP_VALUE(heap, parent_index);
        dheap_assign(heap, sift_index, parent_score, parent_value);
        dheap_assign(heap, parent_index, sift_score, sift_value);
    }
    debug(rb_sprintf("sifted (%"PRIsVALUE", %d, %ld)", heap->values, heap->d, sift_index));
    return LONG2NUM(sift_index);
}

VALUE
dheap_ary_sift_down(dheap_t *heap, long sift_index) {
    VALUE sift_value;
    SCORE sift_score;
    long last_index = DHEAP_IDX_LAST(heap);
    DHEAP_Check_Index(sift_index, last_index);

    sift_value = DHEAP_VALUE(heap, sift_index);
    sift_score = DHEAP_SCORE(heap, sift_index);

     // iteratively sift it down to where it belongs
    for (long child_index; sift_index < last_index; sift_index = child_index) {
        long child_idx0, last_sibidx;
        SCORE child_score;
        VALUE child_value;

        // find first child index, and break if we've reached the last layer
        child_idx0 = child_index = DHEAP_IDX_CHILD0(heap, sift_index);
        debug(rb_sprintf("sift dn(%"PRIsVALUE", %d, %ld)", heap->values, heap->d, sift_index));
        if (last_index < child_idx0) break;

        // find the min child (and its child_index)
        // requires "d" comparisons to find min child and compare to sift_score
        last_sibidx = child_idx0 + heap->d - 1;
        if (last_index < last_sibidx) last_sibidx = last_index;
        child_score = DHEAP_SCORE(heap, child_idx0);
        child_index = child_idx0;
        for (long sibling_index = child_idx0 + 1;
                sibling_index <= last_sibidx;
                ++sibling_index) {
            SCORE sibling_score = DHEAP_SCORE(heap, sibling_index);

            if (CMP_LT(sibling_score, child_score)) {
                child_score = sibling_score;
                child_index = sibling_index;
            }
        }

        // child is larger: heap is restored
        if (CMP_LTE(sift_score, child_score)) break;

        // child is smaller: swap and continue sifting down
        child_value = DHEAP_VALUE(heap, child_index);
        dheap_assign(heap, sift_index, child_score, child_value);
        dheap_assign(heap, child_index, sift_score, sift_value);
    }
    debug(rb_sprintf("sifted (%"PRIsVALUE", %d, %ld)", heap->values, heap->d, sift_index));
    return LONG2NUM(sift_index);
}

/*
 * @return [Integer] the number of elements in the heap
 */
static VALUE
dheap_size(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    long size = DHEAP_SIZE(heap);
    return LONG2NUM(size);
}

/*
 * @return [Boolean] is the heap empty?
 */
static VALUE
dheap_empty_p(VALUE self)
{
    dheap_t *heap = get_dheap_struct(self);
    long size = DHEAP_SIZE(heap);
    return size == 0 ? Qtrue : Qfalse;
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

/*
 * Freezes the heap as well as its underlying array, but does <i>not</i>
 * deep-freeze the elements in the heap.
 *
 * @return [self]
 */
static VALUE
dheap_freeze(VALUE self) {
    dheap_t *heap = get_dheap_struct(self);
    ID id_freeze = rb_intern("freeze");
    rb_funcall(heap->values, id_freeze, 0);
#ifndef SCORE_AS_LONG_DOUBLE
    rb_funcall(heap->scores, id_freeze, 0);
#endif
    return rb_call_super(0, NULL);
}

/* :nodoc: */
static VALUE
dheap_init_clone(VALUE clone, VALUE orig, VALUE kwfreeze)
{
    dheap_initialize_copy(clone, orig);
    if (RTEST(kwfreeze) || (kwfreeze == Qnil && OBJ_FROZEN(orig))) {
        rb_funcall(clone, rb_intern("freeze"), 0);
    }
    return clone;
}

/*
 * @overload push(score, value = score)
 *
 * Push a value onto heap, using a score to determine sort-order.
 *
 * Ideally, the score should be a frozen value that can be efficiently compared
 * to other scores, e.g. an Integer or Float or (maybe) a String
 *
 * Time complexity: <b>O(log n / log d)</b> <i>(worst-case)</i>
 *
 * @param score [#<=>] a value that can be compared to other scores.
 * @param value [Object] an object that is associated with the score.
 *
 * @return [Integer] the index of the value's final position.
 */
static VALUE
dheap_push(int argc, VALUE *argv, VALUE self) {
    VALUE scr, val;
    dheap_t *heap;
    long last_index;
    rb_check_frozen(self);

    rb_check_arity(argc, 1, 2);
    heap = get_dheap_struct(self);
    scr = argv[0];
    val = argc < 2 ? scr : argv[1];

#ifdef SCORE_AS_LONG_DOUBLE
    do {
        long double score_as_ldbl = VAL2SCORE(scr);
        dheap_ensure_room_for_push(heap, 1);
        ++heap->size;
        last_index = DHEAP_IDX_LAST(heap);
        heap->cscores[last_index] = score_as_ldbl;
    } while (0);
#else
    rb_ary_push((heap)->scores, scr);
    last_index = DHEAP_IDX_LAST(heap);
#endif
    rb_ary_push((heap)->values, val);

    return dheap_ary_sift_up(heap, last_index);
}

/*
 * Pushes a comparable value onto the heap.
 *
 * The value will be its own score.
 *
 * Time complexity: <b>O(log n / log d)</b> <i>(worst-case)</i>
 *
 * @param value [#<=>] a value that can be compared to other heap members.
 * @return [self]
 */
static VALUE
dheap_left_shift(VALUE self, VALUE value) {
    dheap_push(1, &value, self);
    return self;
}

#ifdef SCORE_AS_LONG_DOUBLE
    #define DHEAP_DROP_LAST(heap) do {                                   \
        rb_ary_pop(heap->values);                                        \
        --heap->size;                                                    \
    } while (0)
#else
    #define DHEAP_DROP_LAST(heap) do {                                   \
        rb_ary_pop(heap->values);                                        \
        rb_ary_pop(heap->scores);                                        \
    } while (0)
#endif
static inline void
dheap_pop_swap_last_and_sift_down(dheap_t *heap, long last_index)
{
    if (last_index == 0) {
        DHEAP_DROP_LAST(heap);
    }
    else
    {
        VALUE sift_value = DHEAP_VALUE(heap, last_index);
        SCORE sift_score = DHEAP_SCORE(heap, last_index);
        dheap_assign(heap, 0, sift_score, sift_value);
        DHEAP_DROP_LAST(heap);
        dheap_ary_sift_down(heap, 0);
    }
}

#ifdef SCORE_AS_LONG_DOUBLE
    #define DHEAP_CLEAR(heap) do {                                       \
        rb_ary_clear(heap->values);                                      \
        heap->size = 0;                                                  \
    } while (0)
#else
    #define DHEAP_CLEAR(heap) do {                                       \
        rb_ary_clear(heap->values);                                      \
        rb_ary_clear(heap->scores);                                      \
    } while (0)
#endif

/*
 * Returns the next value on the heap to be popped without popping it.
 *
 * Time complexity: <b>O(1)</b> <i>(worst-case)</i>
 * @return [Object] the next value to be popped without popping it.
 */
static VALUE
dheap_clear(VALUE self) {
    dheap_t *heap = get_dheap_struct(self);
    rb_check_frozen(self);
    if (0 < DHEAP_SIZE(heap)) {
        DHEAP_CLEAR(heap);
    }
    return self;
}

/*
 * Returns the next value on the heap to be popped without popping it.
 *
 * Time complexity: <b>O(1)</b> <i>(worst-case)</i>
 * @return [Object] the next value to be popped without popping it.
 */
static VALUE
dheap_peek(VALUE self) {
    dheap_t *heap = get_dheap_struct(self);
    if (DHEAP_IDX_LAST(heap) < 0) return Qnil;
    return DHEAP_VALUE(heap, 0);
}

/*
 * Pops the minimum value from the top of the heap
 *
 * Time complexity: <b>O(d log n / log d)</b> <i>(worst-case)</i>
 */
static VALUE
dheap_pop(VALUE self) {
    VALUE pop_value;
    dheap_t *heap = get_dheap_struct(self);
    long last_index = DHEAP_IDX_LAST(heap);
    rb_check_frozen(self);

    if (last_index < 0) return Qnil;
    pop_value = DHEAP_VALUE(heap, 0);

    dheap_pop_swap_last_and_sift_down(heap, last_index);
    return pop_value;
}

/*
 * Pops the minimum value only if it is less than or equal to a max score.
 *
 * @param max_score [#to_f] the maximum score to be popped
 *
 * @see #pop
 */
static VALUE
dheap_pop_lte(VALUE self, VALUE max_score) {
    VALUE pop_value;
    dheap_t *heap = get_dheap_struct(self);
    long last_index = DHEAP_IDX_LAST(heap);
    rb_check_frozen(self);

    if (last_index <  0) return Qnil;
    pop_value = DHEAP_VALUE(heap, 0);

    do {
        SCORE max = VAL2SCORE(max_score);
        SCORE pop_score = DHEAP_SCORE(heap, 0);
        if (max && !CMP_LTE(pop_score, max)) return Qnil;
    } while (0);

    dheap_pop_swap_last_and_sift_down(heap, last_index);
    return pop_value;
}

/*
 * Pops the minimum value only if it is less than a max score.
 *
 * @param max_score [#to_f] the maximum score to be popped
 *
 * Time complexity: <b>O(d log n / log d)</b> <i>(worst-case)</i>
 */
static VALUE
dheap_pop_lt(VALUE self, VALUE max_score) {
    VALUE pop_value;
    dheap_t *heap = get_dheap_struct(self);
    long last_index = DHEAP_IDX_LAST(heap);
    rb_check_frozen(self);

    if (last_index <  0) return Qnil;
    pop_value = DHEAP_VALUE(heap, 0);

    do {
        SCORE max = VAL2SCORE(max_score);
        SCORE pop_score = DHEAP_SCORE(heap, 0);
        if (max && !CMP_LT(pop_score, max)) return Qnil;
    } while (0);

    dheap_pop_swap_last_and_sift_down(heap, last_index);
    return pop_value;
}

void
Init_d_heap(void)
{
    id_cmp = rb_intern_const("<=>");
    id_abs = rb_intern_const("abs");

    rb_cDHeap = rb_define_class("DHeap", rb_cObject);
    rb_define_alloc_func(rb_cDHeap, dheap_s_alloc);

    rb_define_const(rb_cDHeap, "MAX_D", INT2NUM(DHEAP_MAX_D));
    rb_define_const(rb_cDHeap, "DEFAULT_D", INT2NUM(DHEAP_DEFAULT_D));

    rb_define_method(rb_cDHeap, "initialize", dheap_initialize, -1);
    rb_define_method(rb_cDHeap, "initialize_copy", dheap_initialize_copy, 1);
    rb_define_private_method(rb_cDHeap, "__init_clone__", dheap_init_clone, 2);
    rb_define_method(rb_cDHeap, "freeze", dheap_freeze, 0);

    rb_define_method(rb_cDHeap, "d",       dheap_attr_d, 0);
    rb_define_method(rb_cDHeap, "size",    dheap_size, 0);
    rb_define_method(rb_cDHeap, "empty?",  dheap_empty_p, 0);
    rb_define_method(rb_cDHeap, "peek",    dheap_peek, 0);

    rb_define_method(rb_cDHeap, "clear",   dheap_clear, 0);
    rb_define_method(rb_cDHeap, "push",    dheap_push, -1);
    rb_define_method(rb_cDHeap, "<<",      dheap_left_shift, 1);
    rb_define_method(rb_cDHeap, "pop",     dheap_pop, 0);
    rb_define_method(rb_cDHeap, "pop_lt",  dheap_pop_lt, 1);
    rb_define_method(rb_cDHeap, "pop_lte", dheap_pop_lte, 1);
}
