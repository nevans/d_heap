# frozen_string_literal: true

require "mkmf"

# if /darwin/ =~ RUBY_PLATFORM
#   $CFLAGS << " -D__D_HEAP_DEBUG"
# end

create_makefile("d_heap/d_heap")
