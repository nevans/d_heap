#include "d_heap.h"

ID id_ivar_a;
ID id_ivar_d;

#define DHEAP_Check_d_size(d) \
    if (d < 2) { \
        rb_raise(rb_eIndexError, "DHeap d=%d is too small", d); \
    } \
    if (d > DHEAP_MAX_D) { \
        rb_raise(rb_eIndexError, "DHeap d=%d is too large", d); \
    }

#define DHEAP_Check_Sift_Idx(sift_idx, last_idx) \
    if (sift_idx < 0) { \
        rb_raise(rb_eIndexError, "sift_idx %ld too small", sift_idx); \
    } \
    else if (last_idx < sift_idx) { \
        rb_raise(rb_eIndexError, "sift_idx %ld too large", sift_idx); \
    }

#define DHEAP_Load_Sift_Vals(heap_array, dval, idxval) \
    Check_Type(dval, T_FIXNUM); \
    int d = FIX2INT(dval); \
    long sift_idx = NUM2LONG(idxval);

#define DHEAP_Check_Sift_Args(heap_array, d, sift_idx) \
    DHEAP_Check_d_size(d); \
    Check_Type(heap_array, T_ARRAY); \
    long last_idx = RARRAY_LEN(heap_array) - 1; \
    DHEAP_Check_Sift_Idx(sift_idx, last_idx); \
    \
    VALUE sift_val = rb_ary_entry(heap_array, sift_idx);

VALUE
dheap_ary_sift_up(VALUE heap_array, int d, long sift_idx) {
    DHEAP_Check_Sift_Args(heap_array, d, sift_idx);
    // sift it up to where it belongs
    for (long parent_idx; 0 < sift_idx; sift_idx = parent_idx) {
        parent_idx = (sift_idx - 1) / d;
        VALUE parent_val = rb_ary_entry(heap_array, parent_idx);

        // parent is smaller: heap is restored
        if (CMP_LTE(parent_val, sift_val)) break;

        // parent is larger: swap and continue sifting up
        rb_ary_store(heap_array, sift_idx, parent_val);
        rb_ary_store(heap_array, parent_idx, sift_val);
    }
    return LONG2NUM(sift_idx);
}


VALUE
dheap_ary_sift_down(VALUE heap_array, int d, long sift_idx) {
    DHEAP_Check_Sift_Args(heap_array, d, sift_idx);
     // iteratively sift it down to where it belongs
    for (long child_idx; sift_idx < last_idx; sift_idx = child_idx) {
        // find first child index, and break if we've reached the last layer
        long child_idx0 = sift_idx * d + 1;
        child_idx = child_idx0;
        if (last_idx < child_idx0) break;

        // find the min child (and its child_idx)
        // requires "d" comparisons. (d - 1 to find min child; + 1 vs sift_val)
        VALUE child_val = rb_ary_entry(heap_array, child_idx0);
        for (int i = 1; i < d; i++) {
            long sibling_idx = child_idx0 + i;
            if (last_idx < sibling_idx) break;

            VALUE sibling_val = rb_ary_entry(heap_array, sibling_idx);

            if (CMP_LT(sibling_val, child_val)) {
                child_val = sibling_val;
                child_idx = sibling_idx;
            }
        }

        // child is larger: heap is restored
        if (CMP_GT(child_val, sift_val)) break;

        // child is smaller: swap and continue sifting down
        rb_ary_store(heap_array, sift_idx, child_val);
        rb_ary_store(heap_array, child_idx, sift_val);
    }
    return LONG2NUM(sift_idx);
}

/*
 * call-seq:
 *    DHeap.array_sift_up(heap_array, d, sift_idx)
 *
 * Treats +heap_array+ as a +d+-ary heap and sifts up from +sift_idx+ to restore
 * the heap property.
 *
 * Time complexity: O(d log n / log d).  If the average up shifted element sorts
 * into the bottom layer (e.g. new timers), this can avg O(1).
 *
 */
static VALUE
dheap_sift_up_s(VALUE unused, VALUE heap_array, VALUE dval, VALUE idxval) {
    DHEAP_Load_Sift_Vals(heap_array, dval, idxval);
    return dheap_ary_sift_up(heap_array, d, sift_idx);
}

/*
 * call-seq:
 *    DHeap.array_sift_down(heap_array, d, sift_idx)
 *
 * Treats +heap_array+ as a +d+-ary heap and sifts down from +sift_idx+ to
 * restore the heap property.
 *
 * Time complexity: O(d log n / log d).  If the average down shifted element
 * sorts into the bottom layer (e.g. canceled timers), this can avg O(1).
 *
 */
static VALUE
dheap_sift_down_s(VALUE unused, VALUE heap_array, VALUE dval, VALUE idxval) {
    DHEAP_Load_Sift_Vals(heap_array, dval, idxval);
    return dheap_ary_sift_down(heap_array, d, sift_idx);
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

static inline VALUE dheap_get_a(VALUE dheap) { return rb_ivar_get(dheap, id_ivar_a); }
static inline VALUE dheap_get_d(VALUE dheap) { return rb_ivar_get(dheap, id_ivar_d); }

static VALUE dheap_a(VALUE dheap) { return dheap_get_a(dheap); }
static VALUE dheap_d(VALUE dheap) { return dheap_get_d(dheap); }

/*
 * Push val onto the end of the heap, then sift up to maintain heap property.
 *
 * Returns the index of the value's final position.
 *
 */
static VALUE
dheap_push(VALUE dheap, VALUE val) {
    VALUE heap_a = dheap_get_a(dheap);
    VALUE heap_d = dheap_get_d(dheap);
    rb_ary_push(heap_a, val);
    long last_idx = RARRAY_LEN(heap_a) - 1;
    return dheap_sift_up_s(Qnil, heap_a, heap_d, LONG2NUM(last_idx));
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
dheap_left_shift(VALUE dheap, VALUE val) {
    dheap_push(dheap, val);
    return dheap;
}

/*
 * Pops the minimum value from the top of the heap, sifting down to maintain
 * heap property.
 *
 * Time complexity: O(log n / log d).
 *
 */
static VALUE
dheap_pop(VALUE dheap) {
    VALUE heap_a = dheap_get_a(dheap);
    VALUE heap_d = dheap_get_d(dheap);
    long last_idx = RARRAY_LEN(heap_a) - 1;

    /*
     * short-circuit empty or nearly empty
     */
    if (last_idx <= 0) return rb_ary_pop(heap_a);

    /*
     * swap with last, then sift down
     */
    VALUE pop_val  = rb_ary_entry(heap_a, 0);
    VALUE sift_val = rb_ary_entry(heap_a, last_idx);
    rb_ary_store(heap_a, 0, sift_val);
    rb_ary_pop(heap_a);
    dheap_sift_down_s(Qnil, heap_a, heap_d, LONG2NUM(0));

    return pop_val;
}

void
Init_d_heap(void)
{
    id_cmp = rb_intern_const("<=>");
    id_ivar_a = rb_intern_const("ary");
    id_ivar_d = rb_intern_const("d");

    rb_cDHeap = rb_define_class("DHeap", rb_cObject);
    rb_define_method(rb_cDHeap, "initialize", dheap_initialize, -1);
    rb_define_method(rb_cDHeap, "d", dheap_d, 0);
    rb_define_private_method(rb_cDHeap, "_ary_", dheap_a, 0);

    rb_define_singleton_method(rb_cDHeap, "heap_sift_down", dheap_sift_down_s, 3);
    rb_define_singleton_method(rb_cDHeap, "heap_sift_up",   dheap_sift_up_s, 3);

    rb_define_method(rb_cDHeap, "push", dheap_push, 1);
    rb_define_method(rb_cDHeap, "<<",   dheap_left_shift,  1);
    rb_define_method(rb_cDHeap, "pop",  dheap_pop,  0);
}
