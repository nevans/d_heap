#ifndef D_HEAP_H
#define D_HEAP_H 1

#include "ruby.h"

// d=4 uses the fewest comparisons for insert + delete-min (in the worst case).
#define DHEAP_DEFAULT_D 4

// This is a somewhat arbitary maximum. But benefits from more leaf nodes
// are very unlikely to outweigh the increasinly higher number of worst-case
// comparisons as d gets further from 4.
#define DHEAP_MAX_D 32

#define DHEAP_DEFAULT_SIZE 16
#define DHEAP_MAX_SIZE (LONG_MAX / (int)sizeof(long double))

// 10MB
#define DHEAP_CAPA_INCR_MAX (10 * 1024 * 1024 / (int)sizeof(long double))

VALUE rb_cDHeap;

// copied from pg gem

#define UNUSED(x) ((void)(x))

#ifdef HAVE_RB_GC_MARK_MOVABLE
#define dheap_compact_callback(x) ((void (*)(void*))(x))
#define dheap_gc_location(x) x = rb_gc_location(x)
#else
#define rb_gc_mark_movable(x) rb_gc_mark(x)
#define dheap_compact_callback(x) {(x)}
#define dheap_gc_location(x) UNUSED(x)
#endif

#ifdef __D_HEAP_DEBUG
#define debug(v) { \
    ID sym_puts = rb_intern("puts"); \
    rb_funcall(rb_mKernel, sym_puts, 1, v); \
}
#else
#define debug(v)
#endif

#endif /* D_HEAP_H */
