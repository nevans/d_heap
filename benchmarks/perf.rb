#!/usr/bin/env ruby
# frozen_string_literal: true

require "perf"

system("#{RbConfig.ruby} bin/rake compile", err: :out, exception: true)
require "d_heap/benchmarks"
include DHeap::Benchmarks # rubocop:disable Style/MixinUsage
fill_random_vals

n = ENV.fetch("BENCH_N", 5_000_000).to_i
# interval = ENV.fetch("PROF_INTERVAL", 100).to_i # measured in μs

i = 0

q = initq RbHeap
n.times { q << n }
q.clear

Perf.record do
  while i < n
    q << random_val
    i += 1
  end
  while 0 < i # rubocop:disable Style/NumericPredicate
    q.pop
    i -= 1
  end
end
