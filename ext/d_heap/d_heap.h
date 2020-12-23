#ifndef D_HEAP_H
#define D_HEAP_H 1

#include "ruby.h"

// d=4 uses the fewest comparisons for insert + delete-min (in the worst case).
#define DHEAP_DEFAULT_D 4

// This is a somewhat arbitary maximum. But benefits from more leaf nodes
// are very unlikely to outweigh the increasinly higher number of worst-case
// comparisons as d gets further from 4.
#define DHEAP_MAX_D 32


#define CMP_LT(a, b, cmp_opt) \
    (OPTIMIZED_CMP(a, b, cmp_opt) < 0)
#define CMP_LTE(a, b, cmp_opt) \
    (OPTIMIZED_CMP(a, b, cmp_opt) <= 0)
#define CMP_GT(a, b, cmp_opt) \
    (OPTIMIZED_CMP(a, b, cmp_opt) > 0)
#define CMP_GTE(a, b, cmp_opt) \
    (OPTIMIZED_CMP(a, b, cmp_opt) >= 0)

VALUE rb_cDHeap;
ID id_cmp;

// from internal/numeric.h
#ifndef INTERNAL_NUMERIC_H
int rb_float_cmp(VALUE x, VALUE y);
#endif /* INTERNAL_NUMERIC_H */

// from internal/compar.h
#ifndef INTERNAL_COMPAR_H
#define STRING_P(s) (RB_TYPE_P((s), T_STRING) && CLASS_OF(s) == rb_cString)

enum {
    cmp_opt_Integer,
    cmp_opt_String,
    cmp_opt_Float,
    cmp_optimizable_count
};

struct cmp_opt_data {
    unsigned int opt_methods;
    unsigned int opt_inited;
};

#define NEW_CMP_OPT_MEMO(type, value) \
    NEW_PARTIAL_MEMO_FOR(type, value, cmp_opt)
#define CMP_OPTIMIZABLE_BIT(type) (1U << TOKEN_PASTE(cmp_opt_,type))
#define CMP_OPTIMIZABLE(data, type) \
    (((data).opt_inited & CMP_OPTIMIZABLE_BIT(type)) ? \
     ((data).opt_methods & CMP_OPTIMIZABLE_BIT(type)) : \
     (((data).opt_inited |= CMP_OPTIMIZABLE_BIT(type)), \
      rb_method_basic_definition_p(TOKEN_PASTE(rb_c,type), id_cmp) && \
      ((data).opt_methods |= CMP_OPTIMIZABLE_BIT(type))))

#define OPTIMIZED_CMP(a, b, data) \
    ((FIXNUM_P(a) && FIXNUM_P(b) && CMP_OPTIMIZABLE(data, Integer)) ? \
     (((long)a > (long)b) ? 1 : ((long)a < (long)b) ? -1 : 0) : \
     (STRING_P(a) && STRING_P(b) && CMP_OPTIMIZABLE(data, String)) ? \
     rb_str_cmp(a, b) : \
     (RB_FLOAT_TYPE_P(a) && RB_FLOAT_TYPE_P(b) && CMP_OPTIMIZABLE(data, Float)) ? \
     rb_float_cmp(a, b) : \
     rb_cmpint(rb_funcallv(a, id_cmp, 1, &b), a, b))

#define puts(v) { \
    ID sym_puts = rb_intern("puts"); \
    rb_funcall(rb_mKernel, sym_puts, 1, v); \
}

#endif /* INTERNAL_COMPAR_H */

#endif /* D_HEAP_H */
