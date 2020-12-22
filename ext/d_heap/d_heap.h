#ifndef D_HEAP_H
#define D_HEAP_H 1

#include "ruby.h"

// This is somewhat a arbitary boundary, but it is highly unlikely that the
// gains from fewer levels can outweight doing this many comparisons per level.
// Since the comparisons will still be executed using <=> on ruby objects, it's
// likely they will be too slow to make any d > 8 worthwhile.
#define DHEAP_MAX_D 128
#define DHEAP_DEFAULT_D 4

#define CMP_LT(a, b) \
    (rb_cmpint(rb_funcallv(a, id_cmp, 1, &b), a, b) < 0)
#define CMP_LTE(a, b) \
    (rb_cmpint(rb_funcallv(a, id_cmp, 1, &b), a, b) <= 0)
#define CMP_GT(a, b) \
    (rb_cmpint(rb_funcallv(a, id_cmp, 1, &b), a, b) > 0)
#define CMP_GTE(a, b) \
    (rb_cmpint(rb_funcallv(a, id_cmp, 1, &b), a, b) >= 0)

VALUE rb_cDHeap;
ID id_cmp;

#define puts(v) { \
    ID sym_puts = rb_intern("puts"); \
    rb_funcall(rb_mKernel, sym_puts, 1, v); \
}

VALUE dheap_ary_sift_up(VALUE heap_array, int d, long sift_idx);
VALUE dheap_ary_sift_down(VALUE heap_array, int d, long sift_idx);

#endif /* D_HEAP_H */
