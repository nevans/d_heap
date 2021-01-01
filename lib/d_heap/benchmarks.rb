# frozen_string_literal: true

require "d_heap"
require "ostruct"

# Different benchmark scenarios and implementations to benchmark
module DHeap::Benchmarks # rubocop:disable Style/ClassAndModuleChildren
  TIME_PER_BENCHMARK = ENV.fetch("TIME_PER_BENCHMARK", 5).to_i

  # the maximum expected rate for $dheap_bm_random_vals to be exhausted
  MAX_RATE_PER_SEC = ENV.fetch("MAX_RATE_PER_SEC", 6_000_000).to_i

  OutOfRandomness = Class.new(IndexError)

  def self.puts_version_info(type = "Benchmark", io = $stdout)
    io.puts "#{type} run at %s" % [Time.now]
    io.puts "ruby v%s, DHeap v%s" % [RUBY_VERSION, DHeap::VERSION]
    io.puts
  end

  # rubocop:disable Style/NumericPredicate

  # moves "rand" outside the benchmarked code, to avoid measuring that too.
  module Randomness

    def refill_random_vals(target_size = default_randomness_size, io: $stdout)
      @dheap_bm_random_vals ||= []
      count = target_size - @dheap_bm_random_vals.length
      return if count <= 0
      millions = (count / 1_000_000.0).round(3)
      io.puts "~~~~~~ refilling @dheap_bm_random_vals with #{millions}M ~~~~~~"
      io.flush
      while 0 < count
        @dheap_bm_random_vals << rand(0..10_000)
        count -= 1
      end
    end

    def default_randomness_size
      MAX_RATE_PER_SEC * TIME_PER_BENCHMARK * IMPLEMENTATIONS.count
    end

  end

  # different scenarios to be benchmarked or profiled
  module Scenarios

    def push_n(queue, count)
      while 0 < count
        queue << (@dheap_bm_random_vals.pop or raise OutOfRandomness)
        count -= 1
      end
    end

    def push_n_then_pop_n(queue, count)
      i = 0
      while i < count
        queue << (@dheap_bm_random_vals.pop or raise OutOfRandomness)
        i += 1
      end
      while 0 < i
        queue.pop
        i -= 1
      end
    end

    def repeated_push_pop(queue, count)
      while 0 < count
        queue << (@dheap_bm_random_vals.pop or raise OutOfRandomness)
        queue.pop
        count -= 1
      end
    end

  end

  # rubocop:enable Style/NumericPredicate

  require "d_heap/benchmarks/implementations"

  # Different duck-typed priority queue implemenations
  IMPLEMENTATIONS = [
    OpenStruct.new(name: " push and resort", klass: PushAndResort).freeze,
    OpenStruct.new(name: "bsearch + insert", klass: BinarySearchAndInsert).freeze,
    OpenStruct.new(name: "ruby binary heap", klass: NaiveBinaryHeap).freeze,
    OpenStruct.new(name: "quaternary DHeap", klass: DHeap).freeze,
  ].freeze

end
