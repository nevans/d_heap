# frozen_string_literal: true

require "mkmf"

# For testing in CI (because I don't otherwise have easy access to Mac OS):
# $CFLAGS << " -D__D_HEAP_DEBUG" if /darwin/ =~ RUBY_PLATFORM
# $CFLAGS << " -debug inline-debug-info "
# $CFLAGS << " -g -ginline-points "
# $CFLAGS << " -fno-omit-frame-pointer "

if enable_config("debug") || ENV["EXTCONF_DEBUG"] == "1"
  $stderr.puts "Building in debug mode." # rubocop:disable Style/StderrPuts
  CONFIG["warnflags"] \
    << " -Wall " \
    << " -ggdb" \
    << " -Wpedantic" \
    << " -DDEBUG"
  unless RbConfig::CONFIG["target_os"] =~ /darwin/
    CONFIG["warnflags"] << " -Werror"
  end
end

have_func "rb_gc_mark_movable" # since ruby-2.7

check_sizeof("long")
check_sizeof("unsigned long long")
check_sizeof("long double")
check_sizeof("double")

unless have_macro("LDBL_MANT_DIG", "float.h")
  raise NotImplementedError, "Missing LDBL_MANT_DIG."
end

create_header
create_makefile("d_heap/d_heap")
