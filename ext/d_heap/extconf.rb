# frozen_string_literal: true

require "mkmf"

# For testing in CI (because I don't otherwise have easy access to Mac OS):
# $CFLAGS << " -D__D_HEAP_DEBUG" if /darwin/ =~ RUBY_PLATFORM
# $CFLAGS << " -debug inline-debug-info "
# $CFLAGS << " -g -ginline-points "
# $CFLAGS << " -fno-omit-frame-pointer "

if enable_config("debug")
  CONFIG["warnflags"] << " -Werror -Wpedantic "
end

have_func "rb_gc_mark_movable" # since ruby-2.7

check_sizeof("long")
check_sizeof("unsigned long long")
check_sizeof("long double")
have_macro("LDBL_MANT_DIG", "float.h")

create_makefile("d_heap/d_heap")
