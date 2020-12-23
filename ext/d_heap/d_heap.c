#include "d_heap.h"

ID id_ivar_a;
ID id_ivar_d;

#define DHEAP_GET_A(self) rb_ivar_get(self, id_ivar_a)
#define DHEAP_GET_D(self) rb_ivar_get(self, id_ivar_d)

#define DHEAP_SIZE(ary) (RARRAY_LEN(ary) / 2)
#define DHEAP_LAST_IDX(ary) (DHEAP_SIZE(ary) - 1)
#define DHEAP_VALUE(ary, idx) rb_ary_entry(ary, idx * 2 + 1)
#define DHEAP_SCORE(ary, idx) rb_ary_entry(ary, idx * 2)
#define DHEAP_ASSIGN(ary, idx, scr, val) \
    rb_ary_store(ary, idx * 2,     scr); \
    rb_ary_store(ary, idx * 2 + 1, val);
#define DHEAP_APPEND(ary, scr, val) \
    rb_ary_push(ary, scr); \
    rb_ary_push(ary, val);
#define DHEAP_DROP_LAST(ary) ( \
        rb_ary_pop(ary), \
        rb_ary_pop(ary) \
    ) // score, value
#define IDX_PARENT(idx) ((idx - 1) / d)
#define IDX_CHILD0(idx) ((idx * d) + 1)

#define DHEAP_Check_d_size(d) \
    if (d < 2) { \
        rb_raise(rb_eIndexError, "DHeap d=%d is too small", d); \
    } \
    if (d > DHEAP_MAX_D) { \
        rb_raise(rb_eIndexError, "DHeap d=%d is too large", d); \
    }

#define DHEAP_Check_Sift_Idx(sift_index, last_index) \
    if (sift_index < 0) { \
        rb_raise(rb_eIndexError, "sift_index %ld too small", sift_index); \
    } \
    else if (last_index < sift_index) { \
        rb_raise(rb_eIndexError, "sift_index %ld too large", sift_index); \
    }

#define DHEAP_Check_Sift_Args(heap_array, d, sift_index) \
    DHEAP_Check_d_size(d); \
    Check_Type(heap_array, T_ARRAY); \
    long last_index = DHEAP_LAST_IDX(heap_array); \
    DHEAP_Check_Sift_Idx(sift_index, last_index); \
    \
    VALUE sift_value = DHEAP_VALUE(heap_array, sift_index); \
    VALUE sift_score = DHEAP_SCORE(heap_array, sift_index);

VALUE
dheap_ary_sift_up(VALUE heap_array, int d, long sift_index) {
    DHEAP_Check_Sift_Args(heap_array, d, sift_index);
    struct cmp_opt_data cmp_opt = { 0, 0 };
    // sift it up to where it belongs
    for (long parent_index; 0 < sift_index; sift_index = parent_index) {
        // puts(rb_sprintf("sift up(%"PRIsVALUE", %d, %ld)", heap_array, d, sift_index));
        parent_index = IDX_PARENT(sift_index);
        VALUE parent_score = DHEAP_SCORE(heap_array, parent_index);

        // parent is smaller: heap is restored
        if (CMP_LTE(parent_score, sift_score, cmp_opt)) break;

        // parent is larger: swap and continue sifting up
        VALUE parent_value = DHEAP_VALUE(heap_array, parent_index);
        DHEAP_ASSIGN(heap_array, sift_index, parent_score, parent_value);
        DHEAP_ASSIGN(heap_array, parent_index, sift_score, sift_value);
    }
    // puts(rb_sprintf("sifted (%"PRIsVALUE", %d, %ld)", heap_array, d, sift_index));
    return LONG2NUM(sift_index);
}

VALUE
dheap_ary_sift_down(VALUE heap_array, int d, long sift_index) {
    DHEAP_Check_Sift_Args(heap_array, d, sift_index);
    struct cmp_opt_data cmp_opt = { 0, 0 };

     // iteratively sift it down to where it belongs
    for (long child_index; sift_index < last_index; sift_index = child_index) {
        // puts(rb_sprintf("sift dn(%"PRIsVALUE", %d, %ld)", heap_array, d, sift_index));
        // find first child index, and break if we've reached the last layer
        long child_idx0 = child_index = IDX_CHILD0(sift_index);
        if (last_index < child_idx0) break;

        // find the min child (and its child_index)
        // requires "d" comparisons to find min child and compare to sift_score
        VALUE child_score = DHEAP_SCORE(heap_array, child_idx0);
        child_index = child_idx0;
        for (int i = 1; i < d; ++i) {
            long sibling_index = child_idx0 + i;
            if (last_index < sibling_index) break;

            VALUE sibling_score = DHEAP_SCORE(heap_array, sibling_index);

            if (CMP_LT(sibling_score, child_score, cmp_opt)) {
                child_score = sibling_score;
                child_index = sibling_index;
            }
        }

        // child is larger: heap is restored
        if (CMP_LTE(sift_score, child_score, cmp_opt)) break;

        // child is smaller: swap and continue sifting down
        VALUE child_value = DHEAP_VALUE(heap_array, child_index);
        DHEAP_ASSIGN(heap_array, sift_index, child_score, child_value);
        DHEAP_ASSIGN(heap_array, child_index, sift_score, sift_value);
    }
    // puts(rb_sprintf("sifted (%"PRIsVALUE", %d, %ld)", heap_array, d, sift_index));
    return LONG2NUM(sift_index);
}

#define DHEAP_Load_Sift_Vals(heap_array, dval, idxval) \
    Check_Type(dval, T_FIXNUM); \
    int d = FIX2INT(dval); \
    long sift_index = NUM2LONG(idxval);

/*
 * call-seq:
 *    DHeap.array_sift_up(heap_array, d, sift_index)
 *
 * Treats +heap_array+ as a +d+-ary heap and sifts up from +sift_index+ to restore
 * the heap property.
 *
 * Time complexity: O(d log n / log d).  If the average up shifted element sorts
 * into the bottom layer (e.g. new timers), this can avg O(1).
 *
 */
static VALUE
dheap_sift_up_s(VALUE unused, VALUE heap_array, VALUE dval, VALUE idxval) {
    DHEAP_Load_Sift_Vals(heap_array, dval, idxval);
    return dheap_ary_sift_up(heap_array, d, sift_index);
}

/*
 * call-seq:
 *    DHeap.array_sift_down(heap_array, d, sift_index)
 *
 * Treats +heap_array+ as a +d+-ary heap and sifts down from +sift_index+ to
 * restore the heap property.
 *
 * Time complexity: O(d log n / log d).  If the average down shifted element
 * sorts into the bottom layer (e.g. canceled timers), this can avg O(1).
 *
 */
static VALUE
dheap_sift_down_s(VALUE unused, VALUE heap_array, VALUE dval, VALUE idxval) {
    DHEAP_Load_Sift_Vals(heap_array, dval, idxval);
    return dheap_ary_sift_down(heap_array, d, sift_index);
}

static VALUE
dheap_initialize(int argc, VALUE *argv, VALUE self) {
    rb_check_arity(argc, 0, 1);
    int d = DHEAP_DEFAULT_D;
    if (argc) {
        d = NUM2INT(argv[0]);
    }
    DHEAP_Check_d_size(d);
    rb_ivar_set(self, id_ivar_d, INT2FIX(d));
    rb_ivar_set(self, id_ivar_a, rb_ary_new());
    return self;
}

static VALUE dheap_size(VALUE self) {
    VALUE ary = DHEAP_GET_A(self);
    long size = DHEAP_SIZE(ary);
    return LONG2NUM(size);
}

static VALUE dheap_empty_p(VALUE self) {
    VALUE ary = DHEAP_GET_A(self);
    long size = DHEAP_SIZE(ary);
    return size == 0 ? Qtrue : Qfalse;
}

static VALUE dheap_attr_d(VALUE self) { return DHEAP_GET_D(self); }

static VALUE
dheap_freeze(VALUE self) {
    VALUE ary = DHEAP_GET_A(self);
    ID id_freeze = rb_intern("freeze");
    rb_funcall(ary, id_freeze, 0);
    return rb_call_super(0, NULL);
}

static VALUE
dheap_ary_push(VALUE ary, int d, VALUE val, VALUE scr)
{
    DHEAP_APPEND(ary, scr, val);
    long last_index = DHEAP_LAST_IDX(ary);
    return dheap_ary_sift_up(ary, d, last_index);
}

/*
 * Push val onto the end of the heap, then sift up to maintain heap property.
 *
 * Returns the index of the value's final position.
 *
 */
static VALUE
dheap_push(int argc, VALUE *argv, VALUE self) {
    rb_check_arity(argc, 1, 2);
    VALUE scr  = argv[0];
    VALUE val  = argc < 2 ? scr : argv[1];

    VALUE ary  = DHEAP_GET_A(self);
    VALUE dval = DHEAP_GET_D(self);
    int d = FIX2INT(dval);

    return dheap_ary_push(ary, d, val, scr);
}

/*
 * Push val onto the end of the heap, then sift up to maintain heap property.
 *
 * Time complexity: O(d log n / log d).
 *
 * Returns +self+.
 *
 */
static VALUE
dheap_left_shift(VALUE self, VALUE val) {
    dheap_push(1, &val, self);
    return self;
}

#define DHEAP_Pop_Init(self) \
    VALUE ary  = DHEAP_GET_A(self); \
    VALUE dval = DHEAP_GET_D(self); \
    long last_index = DHEAP_LAST_IDX(ary); \

#define DHEAP_Pop_SwapLastAndSiftDown(ary, dval, last_index, sift_value) \
    if (last_index == 0) { DHEAP_DROP_LAST(ary); return pop_value; } \
    VALUE sift_value = DHEAP_VALUE(ary, last_index); \
    VALUE sift_score = DHEAP_SCORE(ary, last_index); \
    DHEAP_ASSIGN(ary, 0, sift_score, sift_value); \
    DHEAP_DROP_LAST(ary); \
    dheap_ary_sift_down(ary, FIX2INT(dval), 0);

static VALUE
dheap_peek(VALUE self) {
    VALUE ary = DHEAP_GET_A(self);
    return DHEAP_VALUE(ary, 0);
}

/*
 * Pops the minimum value from the top of the heap, sifting down to maintain
 * heap property.
 *
 * Time complexity: O(d log n / log d).
 *
 */
static VALUE
dheap_pop(VALUE self) {
    DHEAP_Pop_Init(self);
    if (last_index <  0) return Qnil;
    VALUE pop_value = DHEAP_VALUE(ary, 0);

    DHEAP_Pop_SwapLastAndSiftDown(ary, dval, last_index, sift_value);
    return pop_value;
}

/*
 * Pops the minimum value from the top of the heap, sifting down to maintain
 * heap property.
 *
 * Time complexity: O(d log n / log d).
 *
 */
static VALUE
dheap_pop_lte(VALUE self, VALUE below_score) {
    DHEAP_Pop_Init(self);
    if (last_index <  0) return Qnil;
    VALUE pop_value = DHEAP_VALUE(ary, 0);

    VALUE pop_score = DHEAP_SCORE(ary, 0);
    struct cmp_opt_data cmp_opt = { 0, 0 };
    if (below_score && !CMP_LTE(pop_score, below_score, cmp_opt)) return Qnil;

    DHEAP_Pop_SwapLastAndSiftDown(ary, dval, last_index, sift_value);
    return pop_value;
}

/*
 * Pops the minimum value from the top of the heap, sifting down to maintain
 * heap property.
 *
 * Time complexity: O(d log n / log d).
 *
 */
static VALUE
dheap_pop_lt(VALUE self, VALUE below_score) {
    DHEAP_Pop_Init(self);
    if (last_index <  0) return Qnil;
    VALUE pop_value = DHEAP_VALUE(ary, 0);

    VALUE pop_score = DHEAP_SCORE(ary, 0);
    struct cmp_opt_data cmp_opt = { 0, 0 };
    if (below_score && !CMP_LT(pop_score, below_score, cmp_opt)) return Qnil;

    DHEAP_Pop_SwapLastAndSiftDown(ary, dval, last_index, sift_value);
    return pop_value;
}

void
Init_d_heap(void)
{
    id_cmp = rb_intern_const("<=>");
    id_ivar_a = rb_intern_const("ary");
    id_ivar_d = rb_intern_const("d");

    rb_cDHeap = rb_define_class("DHeap", rb_cObject);
    rb_define_singleton_method(rb_cDHeap, "heap_sift_down", dheap_sift_down_s, 3);
    rb_define_singleton_method(rb_cDHeap, "heap_sift_up",   dheap_sift_up_s, 3);

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
