# frozen_string_literal: true

require "mkmf"

# rubocop:disable Style/StderrPuts

# For testing in CI (because I don't otherwise have easy access to Mac OS):
# $CFLAGS << " -D__D_HEAP_DEBUG" if /darwin/ =~ RUBY_PLATFORM
# $CFLAGS << " -debug inline-debug-info "
# $CFLAGS << " -g -ginline-points "
# $CFLAGS << " -fno-omit-frame-pointer "

# Allow this to compile on older rubys with C99
if RUBY_VERSION.split(".").first(2).join(".").to_f < 2.7
  $CFLAGS << " -std=c99 -Wno-declaration-after-statement "
end

# Use `rake compile -- --enable-debug`
debug_mode = enable_config("debug", ENV["EXTCONF_DEBUG"] == "1")

# Use `rake compile -- --enable-development`
devel_mode = enable_config("development") || debug_mode

if debug_mode
  $stderr.puts "Building in debug mode."
  $CFLAGS << " -ggdb "
  $CFLAGS << " -DDHEAP_DEBUG "
  $CFLAGS << " -save-temps "
  # $CFLAGS << " -DVERBOSE_DEBUG "
end

if devel_mode
  $stderr.puts "Building in development mode."
  CONFIG["warnflags"] << " -Wall "
  CONFIG["warnflags"] << " -Wpedantic "
  # There are warnings on MacOS that are annoying to debug (I don't have a Mac).
  unless RbConfig::CONFIG["target_os"] =~ /darwin/
    CONFIG["warnflags"] << " -Werror "
  end
end

# Use `rake compile -- --enable-heapmap`
if enable_config("heapmap", true)
  $stderr.puts "Building with DHeap::Map support."
  $defs.push "-DDHEAP_MAP"
end

if enable_config("native-arch", true)
  $stderr.puts "Building with auto-detected architecture support."
  $CFLAGS << " -march=native "
end

min_idx_sse2   = have_const("__SSE2__")
min_idx_avx2   = have_const("__AVX2__")
min_idx_avx512 = have_const("__AVX512F__")

min_idx_sse2   = enable_config("min-idx-sse2",   min_idx_sse2)
min_idx_avx2   = enable_config("min-idx-avx2",   min_idx_avx2)
min_idx_avx512 = enable_config("min-idx-avx512", min_idx_avx512)

if min_idx_sse2 || min_idx_avx2 || min_idx_avx512
  $CFLAGS << " -DDHEAP_SIMD_ENABLED"
  unless have_header("immintrin.h")
    abort "Specify <immintrin> include path or disable SSE2/AVX2/AVX512F."
  end
end

if min_idx_sse2
  $stderr.puts "Enabled SSE2 intrinsics for sift-down min idx."
  $CFLAGS << " -DDHEAP_ENABLE_MIN_IDX_SSE2"
end

if min_idx_avx2
  $stderr.puts "Enabled AVX2 intrinsics for sift-down min idx."
  $CFLAGS << " -DDHEAP_ENABLE_MIN_IDX_AVX2"
end

if min_idx_avx512
  $stderr.puts "Enabled AVX512F intrinsics for sift-down min idx."
  $CFLAGS << " -DDHEAP_ENABLE_MIN_IDX_AVX512"
end

if enable_config("funroll", false)
  $CFLAGS << " -funroll-loops "
end

have_func "rb_gc_mark_movable" # since ruby-2.7

check_sizeof("long")
check_sizeof("unsigned long long")
check_sizeof("long double")
check_sizeof("double")

unless have_macro("LDBL_MANT_DIG", "float.h")
  raise NotImplementedError, "Missing LDBL_MANT_DIG."
end

# rubocop:enable Style/StderrPuts

create_header
create_makefile("d_heap/d_heap")
