# frozen_string_literal: true

require "mkmf"

# if /darwin/ =~ RUBY_PLATFORM
#   $CFLAGS << " -D__D_HEAP_DEBUG"
# end

have_func "rb_gc_mark_movable" # since ruby-2.7

create_makefile("d_heap/d_heap")
