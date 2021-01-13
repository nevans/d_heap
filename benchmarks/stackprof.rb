#!/usr/bin/env ruby
# frozen_string_literal: true

require "stackprof"

system("#{RbConfig.ruby} bin/rake compile", err: :out, exception: true)
require "d_heap/benchmarks"
include DHeap::Benchmarks # rubocop:disable Style/MixinUsage
fill_random_vals

n = ENV.fetch("BENCH_N", 5_000_000).to_i
interval = ENV.fetch("PROF_INTERVAL", 100).to_i # measured in μs

i = 0

q = initq RbHeap
n.times { q << n }
q.clear

StackProf.run(mode: :cpu, out: "stackprof-cpu-push.dump", interval: interval) do
  while i < n
    q << random_val
    i += 1
  end
end
StackProf.run(mode: :cpu, out: "stackprof-cpu-pop.dump", interval: interval) do
  while 0 < i # rubocop:disable Style/NumericPredicate
    q.pop
    i -= 1
  end
end
