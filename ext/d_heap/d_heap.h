#ifndef D_HEAP_H
#define D_HEAP_H 1

#include "ruby.h"

#define DHEAP_DEFAULT_D 4

#define DHEAP_MAX_D INT_MAX

typedef long double SCORE;

typedef struct dheap_entry {
    SCORE score;
    VALUE value;
} ENTRY;

// sizeof(ENTRY) => 32 bytes
// one kilobyte = 32 * 32 bytes
#define DHEAP_DEFAULT_CAPA 32
#define DHEAP_MAX_CAPA (LONG_MAX / (int)sizeof(ENTRY))

#define DHEAP_CAPA_INCR_MAX (10 * 1024 * 1024 / (int)sizeof(ENTRY))

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
