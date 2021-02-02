# frozen_string_literal: true

require "mkmf"

# For testing in CI (because I don't otherwise have easy access to Mac OS):
# $CFLAGS << " -D__D_HEAP_DEBUG" if /darwin/ =~ RUBY_PLATFORM
# $CFLAGS << " -debug inline-debug-info "
# $CFLAGS << " -g -ginline-points "
# $CFLAGS << " -fno-omit-frame-pointer "

# Use `rake compile -- --enable-debug`
debug_mode = enable_config("debug", ENV["EXTCONF_DEBUG"] == "1")

# Use `rake compile -- --enable-development`
devel_mode = enable_config("development") || debug_mode

if debug_mode
  $stderr.puts "Building in debug mode." # rubocop:disable Style/StderrPuts
  CONFIG["warnflags"] \
    << " -ggdb" \
    << " -DDEBUG"
end

if devel_mode
  $stderr.puts "Building in development mode." # rubocop:disable Style/StderrPuts
  CONFIG["warnflags"] \
    << " -Wall " \
    << " -Wpedantic" \
  # There are warnings on MacOS that are annoying to debug (I don't have a Mac).
  unless RbConfig::CONFIG["target_os"] =~ /darwin/
    CONFIG["warnflags"] << " -Werror"
  end
end

# Use `rake compile -- --enable-heapmap`
if enable_config("heapmap", true)
  $stderr.puts "Building with DHeap::Map support." # rubocop:disable Style/StderrPuts
  $defs.push "-DDHEAP_MAP"
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
