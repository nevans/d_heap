#include "d_heap.h"

VALUE rb_mDHeap;

void
Init_d_heap(void)
{
  rb_mDHeap = rb_define_module("DHeap");
}
