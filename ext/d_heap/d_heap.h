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

// copied from pg gem

#ifdef HAVE_RB_GC_MARK_MOVABLE
#define dheap_compact_callback(x) ((void (*)(void*))(x))
#define dheap_gc_location(x) x = rb_gc_location(x)
#else
#define rb_gc_mark_movable(x) rb_gc_mark(x)
#define dheap_compact_callback(x) {(x)}
#define dheap_gc_location(x) UNUSED(x)
#endif

// from internal/compar.h
#define STRING_P(s) (RB_TYPE_P((s), T_STRING) && CLASS_OF(s) == rb_cString)

#ifdef __D_HEAP_DEBUG
#define debug(v) { \
    ID sym_puts = rb_intern("puts"); \
    rb_funcall(rb_mKernel, sym_puts, 1, v); \
}
#else
#define debug(v)
#endif

#endif /* D_HEAP_H */
