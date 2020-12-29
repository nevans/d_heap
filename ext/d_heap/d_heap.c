#include "d_heap.h"

ID id_cmp; // <=>
ID id_ivar_values;
ID id_ivar_scores;
ID id_ivar_d;

#define Get_DHeap(hobj, hstruct) ((hstruct) = get_dheap_struct(hobj))

#define DHEAP_IDX_LAST(heap) (DHEAP_SIZE(heap) - 1)
#define DHEAP_IDX_PARENT(heap, idx) ((idx - 1) / heap->d)
#define DHEAP_IDX_CHILD0(heap, idx) ((idx * heap->d) + 1)

#define DHEAP_SIZE(heap) (RARRAY_LEN((heap)->scores))
#define DHEAP_VALUE(heap, idx) RARRAY_AREF((heap)->values, idx)
#define DHEAP_SCORE(heap, idx) RARRAY_AREF((heap)->scores, idx)
#define DHEAP_ASSIGN(heap, idx, scr, val) \
    rb_ary_store(heap->scores, idx, scr); \
    rb_ary_store(heap->values, idx, val);
#define DHEAP_APPEND(heap, scr, val) \
    rb_ary_push((heap)->scores, scr); \
    rb_ary_push((heap)->values, val);
#define DHEAP_DROP_LAST(heap) ( \
        rb_ary_pop(heap->scores), \
        rb_ary_pop(heap->values) \
    ) // score, value

#define DHEAP_Check_d_size(d) \
    if (d < 2) { \
        rb_raise(rb_eArgError, "DHeap d=%d is too small", d); \
    } \
    if (d > DHEAP_MAX_D) { \
        rb_raise(rb_eArgError, "DHeap d=%d is too large", d); \
    }

#define DHEAP_Check_Index(index, last_index) \
    if (index < 0) { \
        rb_raise(rb_eIndexError, "DHeap index %ld too small", index); \
    } \
    else if (last_index < index) { \
        rb_raise(rb_eIndexError, "DHeap index %ld too large", index); \
    }

struct dheap_struct {
    int d;
    VALUE scores;
    VALUE values;
};
typedef struct dheap_struct dheap_t;

static void
dheap_compact(void *ptr)
{
    dheap_t *heap = ptr;
    if (heap->scores) dheap_gc_location( heap->scores );
    if (heap->values) dheap_gc_location( heap->values );
}

static void
dheap_mark(void *ptr)
{
    dheap_t *heap = ptr;
    if (heap->scores) rb_gc_mark_movable(heap->scores);
    if (heap->values) rb_gc_mark_movable(heap->values);
}

static size_t
dheap_memsize(const void *ptr)
{
    const dheap_t *heap = ptr;
    size_t size = sizeof(*heap);
    return size;
}

static const rb_data_type_t dheap_data_type = {
    "DHeap",
    {
        (void (*)(void*))dheap_mark,
        (void (*)(void*))RUBY_DEFAULT_FREE,
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
    heap->scores = Qnil;
    heap->values = Qnil;

    return obj;
}

static inline dheap_t *
get_dheap_struct(VALUE self)
{
    dheap_t *heap;
    TypedData_Get_Struct(self, dheap_t, &dheap_data_type, heap);
    Check_Type(heap->scores, T_ARRAY);
    return heap;
}

/*
 * @overload initialize(d = DHeap::DEFAULT_D)
 *   Initialize a _d_-ary min-heap.
 *
 *   @param d [Integer] maximum number of children per parent
 */
static VALUE
dheap_initialize(int argc, VALUE *argv, VALUE self) {
    rb_check_arity(argc, 0, 1);
    dheap_t *heap;
    TypedData_Get_Struct(self, dheap_t, &dheap_data_type, heap);

    int d = DHEAP_DEFAULT_D;
    if (argc) {
        d = NUM2INT(argv[0]);
    }
    DHEAP_Check_d_size(d);
    heap->d = d;

    heap->scores = rb_ary_new_capa(10000);
    heap->values = rb_ary_new_capa(10000);

    return self;
}

/*
static inline VALUE
make_dheap_element(VALUE score, VALUE value)
{
    elem_t *elem;
    VALUE obj = TypedData_Make_Struct(
            rb_cObject,
            elem_t,
            &dheap_elem_type,
            elem);
    elem->score = score;
    elem->value = value;
    return obj;
}

#define IsDHeapElem(value) rb_typeddata_is_kind_of((value), &dheap_elem_type)
#define Get_DHeap_Elem(value) \
    TypedData_Get_Struct((obj), elem_t, &dheap_elem_type, (elem))

static inline elem_t *
get_dheap_element(VALUE obj)
{
    elem_t *elem;
    TypedData_Get_Struct((obj), elem_t, &dheap_elem_type, (elem));
    return elem;
}

*/

#define CMP_LT(a, b)  (optimized_cmp(a, b) <  0)
#define CMP_LTE(a, b) (optimized_cmp(a, b) <= 0)
#define CMP_GT(a, b)  (optimized_cmp(a, b) >  0)
#define CMP_GTE(a, b) (optimized_cmp(a, b) >= 0)

/*
 * short-circuit evaluation for a few basic types.
 *
 * Only Integer, Float, and String are optimized,
 * and only when both arguments are the same type.
 */
static inline int
optimized_cmp(VALUE a, VALUE b) {
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

VALUE
dheap_ary_sift_up(dheap_t *heap, long sift_index) {
    long last_index = DHEAP_IDX_LAST(heap);
    DHEAP_Check_Index(sift_index, last_index);

    VALUE sift_value = DHEAP_VALUE(heap, sift_index);
    VALUE sift_score = DHEAP_SCORE(heap, sift_index);

    // sift it up to where it belongs
    for (long parent_index; 0 < sift_index; sift_index = parent_index) {
        debug(rb_sprintf("sift up(%"PRIsVALUE", %d, %ld)", heap->values, heap->d, sift_index));
        parent_index = DHEAP_IDX_PARENT(heap, sift_index);
        VALUE parent_score = DHEAP_SCORE(heap, parent_index);

        // parent is smaller: heap is restored
        if (CMP_LTE(parent_score, sift_score)) break;

        // parent is larger: swap and continue sifting up
        VALUE parent_value = DHEAP_VALUE(heap, parent_index);
        DHEAP_ASSIGN(heap, sift_index, parent_score, parent_value);
        DHEAP_ASSIGN(heap, parent_index, sift_score, sift_value);
    }
    debug(rb_sprintf("sifted (%"PRIsVALUE", %d, %ld)", heap->values, heap->d, sift_index));
    return LONG2NUM(sift_index);
}

VALUE
dheap_ary_sift_down(dheap_t *heap, long sift_index) {
    long last_index = DHEAP_IDX_LAST(heap);
    DHEAP_Check_Index(sift_index, last_index);

    VALUE sift_value = DHEAP_VALUE(heap, sift_index);
    VALUE sift_score = DHEAP_SCORE(heap, sift_index);

     // iteratively sift it down to where it belongs
    for (long child_index; sift_index < last_index; sift_index = child_index) {
        debug(rb_sprintf("sift dn(%"PRIsVALUE", %d, %ld)", heap->values, heap->d, sift_index));
        // find first child index, and break if we've reached the last layer
        long child_idx0 = child_index = DHEAP_IDX_CHILD0(heap, sift_index);
        if (last_index < child_idx0) break;

        // find the min child (and its child_index)
        // requires "d" comparisons to find min child and compare to sift_score
        long last_sibidx = child_idx0 + heap->d - 1;
        if (last_index < last_sibidx) last_sibidx = last_index;
        VALUE child_score = DHEAP_SCORE(heap, child_idx0);
        child_index = child_idx0;
        for (long sibling_index = child_idx0 + 1;
                sibling_index <= last_sibidx;
                ++sibling_index) {
            VALUE sibling_score = DHEAP_SCORE(heap, sibling_index);

            if (CMP_LT(sibling_score, child_score)) {
                child_score = sibling_score;
                child_index = sibling_index;
            }
        }

        // child is larger: heap is restored
        if (CMP_LTE(sift_score, child_score)) break;

        // child is smaller: swap and continue sifting down
        VALUE child_value = DHEAP_VALUE(heap, child_index);
        DHEAP_ASSIGN(heap, sift_index, child_score, child_value);
        DHEAP_ASSIGN(heap, child_index, sift_score, sift_value);
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
    rb_funcall(heap->scores, id_freeze, 0);
    rb_funcall(heap->values, id_freeze, 0);
    return rb_call_super(0, NULL);
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
    rb_check_arity(argc, 1, 2);
    VALUE scr  = argv[0];
    VALUE val  = argc < 2 ? scr : argv[1];

    dheap_t *heap = get_dheap_struct(self);

    DHEAP_APPEND(heap, scr, val);
    long last_index = DHEAP_IDX_LAST(heap);
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

/*
    dheap_t hstruct; \
    Get_DHeap(self, hstruct); \
    VALUE values = hstruct.values; \
    VALUE scores = hstruct.scores; \
    VALUE dval = INT2FIX(hstruct.d); \
*/

static inline void
dheap_pop_swap_last_and_sift_down(dheap_t *heap, long last_index)
{
    if (last_index == 0) {
        DHEAP_DROP_LAST(heap);
    }
    else
    {
        VALUE sift_value = DHEAP_VALUE(heap, last_index);
        VALUE sift_score = DHEAP_SCORE(heap, last_index);
        DHEAP_ASSIGN(heap, 0, sift_score, sift_value);
        DHEAP_DROP_LAST(heap);
        dheap_ary_sift_down(heap, 0);
    }
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
    dheap_t *heap = get_dheap_struct(self);
    long last_index = DHEAP_IDX_LAST(heap);

    if (last_index < 0) return Qnil;
    VALUE pop_value = DHEAP_VALUE(heap, 0);

    dheap_pop_swap_last_and_sift_down(heap, last_index);
    return pop_value;
}

/*
 * Pops the minimum value only if it is less than or equal to a max score.
 *
 * @param max_score [#<=>] the maximum score to be popped
 *
 * @see #pop
 */
static VALUE
dheap_pop_lte(VALUE self, VALUE max_score) {
    dheap_t *heap = get_dheap_struct(self);
    long last_index = DHEAP_IDX_LAST(heap);

    if (last_index <  0) return Qnil;
    VALUE pop_value = DHEAP_VALUE(heap, 0);

    VALUE pop_score = DHEAP_SCORE(heap, 0);
    if (max_score && !CMP_LTE(pop_score, max_score)) return Qnil;

    dheap_pop_swap_last_and_sift_down(heap, last_index);
    return pop_value;
}

/*
 * Pops the minimum value only if it is less than a max score.
 *
 * @param max_score [#<=>] the maximum score to be popped
 *
 * Time complexity: <b>O(d log n / log d)</b> <i>(worst-case)</i>
 */
static VALUE
dheap_pop_lt(VALUE self, VALUE max_score) {
    dheap_t *heap = get_dheap_struct(self);
    long last_index = DHEAP_IDX_LAST(heap);

    if (last_index <  0) return Qnil;
    VALUE pop_value = DHEAP_VALUE(heap, 0);

    VALUE pop_score = DHEAP_SCORE(heap, 0);
    if (max_score && !CMP_LT(pop_score, max_score)) return Qnil;

    dheap_pop_swap_last_and_sift_down(heap, last_index);
    return pop_value;
}

void
Init_d_heap(void)
{
    id_cmp = rb_intern_const("<=>");
    id_ivar_values = rb_intern_const("values");
    id_ivar_scores = rb_intern_const("scores");
    id_ivar_d = rb_intern_const("d");

    rb_cDHeap = rb_define_class("DHeap", rb_cObject);
    rb_define_alloc_func(rb_cDHeap, dheap_s_alloc);

    rb_define_const(rb_cDHeap, "MAX_D", INT2NUM(DHEAP_MAX_D));
    rb_define_const(rb_cDHeap, "DEFAULT_D", INT2NUM(DHEAP_DEFAULT_D));

    rb_define_method(rb_cDHeap, "initialize", dheap_initialize, -1);
    rb_define_method(rb_cDHeap, "d", dheap_attr_d, 0);
    rb_define_method(rb_cDHeap, "freeze", dheap_freeze, 0);

    rb_define_method(rb_cDHeap, "size", dheap_size, 0);
    rb_define_method(rb_cDHeap, "empty?", dheap_empty_p, 0);

    rb_define_method(rb_cDHeap, "peek",    dheap_peek, 0);
    rb_define_method(rb_cDHeap, "push",    dheap_push, -1);
    rb_define_method(rb_cDHeap, "<<",      dheap_left_shift, 1);
    rb_define_method(rb_cDHeap, "pop",     dheap_pop, 0);
    rb_define_method(rb_cDHeap, "pop_lt",  dheap_pop_lt, 1);
    rb_define_method(rb_cDHeap, "pop_lte", dheap_pop_lte, 1);
}
