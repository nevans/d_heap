# frozen_string_literal: true

require "mkmf"

# For testing in CI (because I don't otherwise have easy access to Mac OS):
# $CFLAGS << " -D__D_HEAP_DEBUG" if /darwin/ =~ RUBY_PLATFORM

have_func "rb_gc_mark_movable" # since ruby-2.7

check_sizeof("long")
check_sizeof("unsigned long long")
check_sizeof("long double")
have_macro("LDBL_MANT_DIG", "float.h")

CONFIG["warnflags"] << " -Werror"
create_makefile("d_heap/d_heap")
