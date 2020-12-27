#ifndef D_HEAP_H
#define D_HEAP_H 1

#include "ruby.h"

// d=4 uses the fewest comparisons for insert + delete-min (in the worst case).
#define DHEAP_DEFAULT_D 4

// This is a somewhat arbitary maximum. But benefits from more leaf nodes
// are very unlikely to outweigh the increasinly higher number of worst-case
// comparisons as d gets further from 4.
#define DHEAP_MAX_D 32

VALUE rb_cDHeap;

#define CMP_LT(a, b)  (optimized_cmp(a, b) <  0)
#define CMP_LTE(a, b) (optimized_cmp(a, b) <= 0)
#define CMP_GT(a, b)  (optimized_cmp(a, b) >  0)
#define CMP_GTE(a, b) (optimized_cmp(a, b) >= 0)

// <=>
ID id_cmp;

// from internal/compar.h
#define STRING_P(s) (RB_TYPE_P((s), T_STRING) && CLASS_OF(s) == rb_cString)

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

#define puts(v) { \
    ID sym_puts = rb_intern("puts"); \
    rb_funcall(rb_mKernel, sym_puts, 1, v); \
}

#endif /* D_HEAP_H */
