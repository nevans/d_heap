#include <float.h>
#include "d_heap.h"

ID id_cmp; // <=>
ID id_abs; // abs
ID id_unary_minus; // -@

typedef struct dheap_struct {
    int d;
    long size;
    long capa;
    ENTRY *entries;
} dheap_t;

#define DHEAP_SCORE(heap, idx) ((heap)->entries[idx].score)
#define DHEAP_VALUE(heap, idx) ((heap)->entries[idx].value)
#define DHEAP_IDX_LAST(heap) (DHEAP_SIZE((heap)) - 1)
#define DHEAP_IDX_PARENT(heap, idx) (((idx) - 1) / (heap)->d)
#define DHEAP_IDX_CHILD_0(heap, idx) (((idx) * (heap)->d) + 1)
#define DHEAP_IDX_CHILD_D(heap, idx) (((idx) * (heap)->d) + (heap)->d)

#define DHEAP_SIZE(heap) ((heap)->size)

#define CMP_LT(a, b)  ((a) <  (b))
#define CMP_LTE(a, b) ((a) <= (b))
#define CMP_GT(a, b)  ((a) >  (b))
#define CMP_GTE(a, b) ((a) >= (b))

#if LDBL_MANT_DIG < SIZEOF_UNSIGNED_LONG_LONG * 8
#error 'unsigned long long' should fit into 'long double' mantissa
#endif

// ruby doesn't have a LDBL2NUM. :(
// So this only accomplishes a subset of what that ought to do.
static inline VALUE
SCORE2NUM(SCORE s)
{
    if (floorl((long double) s) == s) {
        if (s < 0) {
            unsigned long long ull = (unsigned long long)(-s);
            return rb_funcall(ULL2NUM(ull), id_unary_minus, 0);
        }
        return ULL2NUM((unsigned long long)(s));
    }
    return rb_float_new((double)(s));
}

static inline VALUE
DHEAP_ENTRY_ARY(dheap_t *heap, long idx)
{
    if (idx < 0 || heap->size <= idx) { return Qnil; }
    return rb_ary_new_from_args(2,
            DHEAP_VALUE(heap, 0),
            SCORE2NUM(DHEAP_SCORE(heap, 0)));
}

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

#ifdef ORIG_SCORE_CMP_CODE

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

#ifdef __D_HEAP_DEBUG
#define ASSERT_DHEAP_INDEX(heap, index) do {                           \
    if (index < 0) {                                                  \
        rb_raise(rb_eIndexError, "DHeap index %ld too small", index); \
    }                                                                 \
    else if (DHEAP_IDX_LAST(heap) < index) {                          \
        rb_raise(rb_eIndexError, "DHeap index %ld too large", index); \
    }                                                                 \
} while (0)
#else
#define ASSERT_DHEAP_INDEX(heap, index)
#endif

static void
dheap_compact(void *ptr)
{
    dheap_t *heap = ptr;
    for (long i = 0; i < heap->size; ++i) {
        if (DHEAP_VALUE(heap, i))
            dheap_gc_location(DHEAP_VALUE(heap, i));
    }
}

static void
dheap_mark(void *ptr)
{
    dheap_t *heap = ptr;
    for (long i = 0; i < heap->size; ++i) {
        if (DHEAP_VALUE(heap, i))
            rb_gc_mark_movable(DHEAP_VALUE(heap,i));
    }
}

static void
dheap_free(void *ptr)
{
    dheap_t *heap = ptr;
    heap->size = 0;
    if (heap->entries) {
        xfree(heap->entries);
        heap->entries = NULL;
    }
    heap->capa = 0;
    xfree(ptr);
}

static size_t
dheap_memsize(const void *ptr)
{
    const dheap_t *heap = ptr;
    size_t size = 0;
    size += sizeof(*heap);
    size += sizeof(ENTRY) * heap->capa;
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

    heap->size = 0;
    heap->capa = 0;
    heap->entries = NULL;

    return obj;
}

static inline dheap_t *
get_dheap_struct(VALUE self)
{
    dheap_t *heap;
    TypedData_Get_Struct(self, dheap_t, &dheap_data_type, heap);
    return heap;
}

void
dheap_set_capa(dheap_t *heap, long new_capa)
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
dheap_ensure_room_for_push(dheap_t *heap, long incr_by)
{
    long new_size = heap->size + incr_by;

    // check for overflow of new_size
    if (DHEAP_MAX_CAPA - incr_by < heap->size)
        rb_raise(rb_eIndexError, "index %ld too big", new_size);

    // if it existing capacity is too small
    if (heap->capa < new_size) {
        // double it...
        long new_capa = new_size * 2;
        if (DHEAP_CAPA_INCR_MAX < new_size)
            new_size = new_size + DHEAP_CAPA_INCR_MAX;
        // check for overflow of new_capa
        if (DHEAP_MAX_CAPA / 2 < new_size) new_capa = DHEAP_MAX_CAPA;
        // cap max incr_by
        if (heap->capa + DHEAP_CAPA_INCR_MAX < new_capa)
            new_capa = heap->capa + DHEAP_CAPA_INCR_MAX;

        dheap_set_capa(heap, new_capa);
    }
}

static inline int
dheap_value_to_int_d(VALUE num)
{
    int d = NUM2INT(num);
    if (d < 2) {
        rb_raise(rb_eArgError, "DHeap d=%u is too small", d);
    }
    if (d > DHEAP_MAX_D) {
        rb_raise(rb_eArgError, "DHeap d=%u is too large", d);
    }
    return d;
}

static inline long
dheap_value_to_long_capa(VALUE num)
{
    long capa = NUM2LONG(num);
    if (capa < 1) {
        rb_raise(rb_eArgError, "DHeap capa=%lu must be positive", capa);
    }
    return capa;
}

static VALUE
dheap_initialize(VALUE self, VALUE d, VALUE capa) {
    dheap_t *heap = get_dheap_struct(self);

    if(heap->entries || heap->size || heap->capa)
        rb_raise(rb_eScriptError, "DHeap already initialized.");

    heap->d = dheap_value_to_int_d(d);
    dheap_set_capa(heap, dheap_value_to_long_capa(capa));

    return self;
}

/* @!visibility private */
static VALUE
dheap_initialize_copy(VALUE copy, VALUE orig)
{
    dheap_t *heap_copy;
    dheap_t *heap_orig = get_dheap_struct(orig);

    rb_check_frozen(copy);
    TypedData_Get_Struct(copy, dheap_t, &dheap_data_type, heap_copy);

    heap_copy->d = heap_orig->d;

    dheap_set_capa(heap_copy, heap_orig->capa);
    heap_copy->size = heap_orig->size;
    if (heap_copy->size)
        MEMCPY(heap_copy->entries, heap_orig->entries, ENTRY, heap_orig->size);

    return copy;
}

VALUE
dheap_sift_up(dheap_t *heap, long index) {
    ENTRY entry = heap->entries[index];

    ASSERT_DHEAP_INDEX(heap, index);

    // sift it up to where it belongs
    for (long parent_index; 0 < index; index = parent_index) {
        /* debug(rb_sprintf("sift up(%"PRIsVALUE", %d, %ld)", heap->values, heap->d, index)); */
        parent_index = DHEAP_IDX_PARENT(heap, index);

        // parent is smaller: heap is restored
        if (CMP_LTE(DHEAP_SCORE(heap, parent_index), entry.score)) break;

        // parent is larger: swap and continue sifting up
        heap->entries[index] = heap->entries[parent_index];
    }
    heap->entries[index] = entry;
    /* debug(rb_sprintf("sifted (%"PRIsVALUE", %d, %ld)", heap->values, heap->d, index)); */
    return LONG2NUM(index);
}

/*
 * this is a tiny bit more complicated than the binary heap version
 */
static inline long
dheap_find_min_child(dheap_t *heap, long parent, long last_index) {
    long min_child = DHEAP_IDX_CHILD_0(heap, parent);
    long last_sib  = DHEAP_IDX_CHILD_D(heap, parent);
    if (last_index < last_sib) last_sib = last_index;

    for (long sibidx = min_child + 1; sibidx <= last_sib; ++sibidx) {
        if (CMP_LT(DHEAP_SCORE(heap, sibidx),
                    DHEAP_SCORE(heap, min_child))) {
            min_child = sibidx;
        }
    }
    return min_child;
}

void
dheap_sift_down(dheap_t *heap, long index, long last_index) {
    if (last_index < 1 || DHEAP_IDX_PARENT(heap, last_index) < index) {
        // short-circuit: no chance for a child
        return;

    } else {
        ENTRY entry = heap->entries[index];

        long last_parent = DHEAP_IDX_PARENT(heap, last_index);

        ASSERT_DHEAP_INDEX(heap, index);

        // iteratively sift it down to where it belongs
        while (index <= last_parent) {
            // find min child
            long min_child = dheap_find_min_child(heap, index, last_index);

            // child is larger: heap is restored
            if (CMP_LTE(entry.score, DHEAP_SCORE(heap, min_child))) break;

            // child is smaller: swap and continue sifting down
            heap->entries[index] = heap->entries[min_child];
            index = min_child;
        }
        heap->entries[index] = entry;
        // debug(rb_sprintf("sifted (%"PRIsVALUE", %d, %ld)", heap->values, heap->d, index));
    }
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
 * @return [Boolean] if the heap is empty
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

static inline void
dheap_push_entry(dheap_t *heap, ENTRY *entry) {
    dheap_ensure_room_for_push(heap, 1);
    heap->entries[heap->size] = *entry;
    ++heap->size;
    dheap_sift_up(heap, heap->size - 1);
}

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
dheap_insert(VALUE self, VALUE score, VALUE value) {
    ENTRY entry;
    dheap_t *heap = get_dheap_struct(self);
    rb_check_frozen(self);

    entry.score = VAL2SCORE(score);
    entry.value = value;
    dheap_push_entry(heap, &entry);

    return self;
}

/*
 * @overload push(value, score = value)
 *
 * Push a value onto heap, using a score to determine sort-order.
 *
 * Value comes first because the separate score is optional, and because it feels
 * like a more natural variation on +Array#push+ or +Queue#enq+.  If a score
 * isn't provided, the value must be an Integer or can be cast with
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
dheap_push(int argc, VALUE *argv, VALUE self) {
    dheap_t *heap = get_dheap_struct(self);
    ENTRY entry;
    rb_check_frozen(self);

    rb_check_arity(argc, 1, 2);
    entry.value = argv[0];
    entry.score = VAL2SCORE(argc < 2 ? entry.value : argv[1]);
    dheap_push_entry(heap, &entry);

    return self;
}

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
dheap_left_shift(VALUE self, VALUE value) {
    dheap_t *heap = get_dheap_struct(self);
    ENTRY entry;
    rb_check_frozen(self);

    entry.score = VAL2SCORE(value);
    entry.value = value;
    dheap_push_entry(heap, &entry);

    return self;
}

static const ENTRY EmptyDheapEntry;

static inline void
dheap_del0(dheap_t *heap)
{
    if (0 < --heap->size) {
        heap->entries[0] = heap->entries[heap->size];
        heap->entries[heap->size] = EmptyDheapEntry; // unnecessary to zero?
        dheap_sift_down(heap, 0, heap->size - 1);
    }
}

static inline VALUE
dheap_pop0(dheap_t *heap)
{
    VALUE popped = DHEAP_VALUE(heap, 0);
    dheap_del0(heap);
    return popped;
}

/*
 * Clears all values from the heap, leaving it empty.
 *
 * @return [self]
 */
static VALUE
dheap_clear(VALUE self) {
    dheap_t *heap = get_dheap_struct(self);
    rb_check_frozen(self);
    if (0 < DHEAP_SIZE(heap)) {
        heap->size = 0;
    }
    return self;
}

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
    return DHEAP_ENTRY_ARY(heap, 0);
}

/*
 * Returns the next score on the heap, without the value and without popping it.
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
    if (DHEAP_IDX_LAST(heap) < 0) return Qnil;
    return SCORE2NUM(DHEAP_SCORE(heap, 0));
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
    if (DHEAP_IDX_LAST(heap) < 0) return Qnil;
    return DHEAP_VALUE(heap, 0);
}

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
    dheap_t *heap = get_dheap_struct(self);
    rb_check_frozen(self);
    if (DHEAP_SIZE(heap) <= 0) return Qnil;
    return dheap_pop0(heap);
}

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
    dheap_t *heap = get_dheap_struct(self);
    VALUE ary = DHEAP_ENTRY_ARY(heap, 0);
    rb_check_frozen(self);
    if (ary != Qnil) { dheap_pop0(heap); }
    return ary;
}

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
 */
static VALUE
dheap_pop_lte(VALUE self, VALUE max_score)
{
    dheap_t *heap = get_dheap_struct(self);
    rb_check_frozen(self);
    if (DHEAP_SIZE(heap) <= 0) return Qnil;
    if (!CMP_LTE(DHEAP_SCORE(heap, 0), VAL2SCORE(max_score))) return Qnil;
    return dheap_pop0(heap);
}

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
 */
static VALUE
dheap_pop_lt(VALUE self, VALUE max_score)
{
    dheap_t *heap = get_dheap_struct(self);
    rb_check_frozen(self);
    if (DHEAP_SIZE(heap) <= 0) return Qnil;
    if (!CMP_LT(DHEAP_SCORE(heap, 0), VAL2SCORE(max_score))) return Qnil;
    return dheap_pop0(heap);
}

void
Init_d_heap(void)
{
    id_cmp = rb_intern_const("<=>");
    id_abs = rb_intern_const("abs");
    id_unary_minus = rb_intern_const("-@");

    rb_cDHeap = rb_define_class("DHeap", rb_cObject);
    rb_define_alloc_func(rb_cDHeap, dheap_s_alloc);

    /*
     * This is based on INT_MAX. But it is very very unlikely you will want a
     * large value for d.  The tradeoff is that higher d values give faster push
     * and slower pop.  If you expect pushes and pops to be balanced, then just
     * stick with the default.  If you expect more pushes than pops, it might be
     * worthwhile to increase d.
     */
    rb_define_const(rb_cDHeap, "MAX_D",        INT2NUM(DHEAP_MAX_D));

    /*
     * d=4 uses the fewest comparisons for (worst case) insert + delete-min.
     */
    rb_define_const(rb_cDHeap, "DEFAULT_D",    INT2NUM(DHEAP_DEFAULT_D));

    /*
     * The default heap capacity.  The heap grows automatically as necessary, so
     * you shouldn't need to worry about this.
     */
    rb_define_const(rb_cDHeap, "DEFAULT_CAPA", INT2NUM(DHEAP_DEFAULT_CAPA));

    rb_define_private_method(rb_cDHeap, "__init_without_kw__", dheap_initialize, 2);
    rb_define_method(rb_cDHeap, "initialize_copy", dheap_initialize_copy, 1);

    rb_define_method(rb_cDHeap, "d",       dheap_attr_d, 0);
    rb_define_method(rb_cDHeap, "size",    dheap_size, 0);
    rb_define_method(rb_cDHeap, "empty?",  dheap_empty_p, 0);
    rb_define_method(rb_cDHeap, "peek",    dheap_peek, 0);

    rb_define_method(rb_cDHeap, "clear",   dheap_clear, 0);
    rb_define_method(rb_cDHeap, "insert",  dheap_insert, 2);
    rb_define_method(rb_cDHeap, "push",    dheap_push, -1);
    rb_define_method(rb_cDHeap, "<<",      dheap_left_shift, 1);
    rb_define_method(rb_cDHeap, "pop",     dheap_pop, 0);
    rb_define_method(rb_cDHeap, "pop_lt",  dheap_pop_lt, 1);
    rb_define_method(rb_cDHeap, "pop_lte", dheap_pop_lte, 1);

    rb_define_method(rb_cDHeap, "peek_score",      dheap_peek_score, 0);
    rb_define_method(rb_cDHeap, "peek_with_score", dheap_peek_with_score, 0);
    rb_define_method(rb_cDHeap, "pop_with_score",  dheap_pop_with_score, 0);

}
