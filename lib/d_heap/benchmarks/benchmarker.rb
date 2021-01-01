# frozen_string_literal: true

require "d_heap/benchmarks"

require "benchmark/ips"

module DHeap::Benchmarks # rubocop:disable Style/ClassAndModuleChildren
  # Benchmarks different implementations with different sizes
  class Benchmarker
    include Randomness
    include Scenarios

    N_COUNTS = [3, 7, 15, 31, 100, 1000, 10_000, 100_000].freeze

    attr_reader :time
    attr_reader :iterations_for_push_pop
    attr_reader :io

    def initialize(
      time: Integer(ENV.fetch("BENCHMARK_TIME", 10)),
      iterations_for_push_pop: 10_000,
      io: $stdout
    )
      @time = time
      @iterations_for_push_pop = Integer(iterations_for_push_pop)
      @io = io
    end

    def call(queue_size: ENV.fetch("BENCHMARK_QUEUE_SIZE", :unset))
      DHeap::Benchmarks.puts_version_info("Benchmarking")
      run_tests(io: io) unless ENV["SKIP_TESTS"] == "1"
      sizes = (queue_size == :unset) ? N_COUNTS : [queue_size]
      sizes.each do |size|
        benchmark_size(size)
      end
    end

    def benchmark_size(queue_size)
      io.puts <<~TEXT
        ########################################################################
        # Benchmarks with N=#{queue_size} (t=#{time}sec/benchmark)
        ########################################################################

      TEXT
      benchmark_push_n            queue_size
      benchmark_push_n_then_pop_n queue_size
      benchmark_repeated_push_pop queue_size
    end

    def benchmark_push_n(queue_size)
      benchmarking("push N", queue_size, init: 0) do |queue|
        push_n(queue, queue_size)
      end
    end

    def benchmark_push_n_then_pop_n(queue_size)
      benchmarking("push N then pop N", queue_size, init: 0) do |queue|
        push_n_then_pop_n(queue, queue_size)
      end
    end

    def benchmark_repeated_push_pop(queue_size)
      benchmark_name = "Push/pop %d with pre-filled queue (size=N)" % [
        iterations_for_push_pop,
      ]
      benchmarking(benchmark_name, queue_size, init: queue_size) do |queue|
        push_n_then_pop_n(queue, iterations_for_push_pop)
      end
    end

    private

    # TODO: move somewhere else...
    def skip_profiling?(queue_size, impl)
      impl.klass == DHeap::Benchmarks::PushAndResort && 10_000 < queue_size
    end

    # rubocop:disable Metrics/MethodLength, Metrics/AbcSize, Style/MultilineBlockChain

    def implementations_with_queues(init)
      IMPLEMENTATIONS.reject {|impl|
        skip_profiling?(init, impl)
      }.map {|impl|
        queue = impl.klass.new
        unless init.zero?
          puts "Pre-filling #{impl.name} priority queue ---------------------"
          push_n(queue, init)
        end
        [impl, queue]
      }
    end

    def benchmarking(name, queue_size, init: 0)
      refill_random_vals
      impl_queues = implementations_with_queues(init)
      refill_random_vals
      puts "== #{name} (N=#{queue_size}) ================================="
      Benchmark.ips do |bm|
        bm.config(time: time, warmup: 0)
        impl_queues.each do |impl, queue|
          next if skip_profiling?(queue_size, impl)
          bm.report(impl.name) do
            yield queue
          end
          bm.compare!
        end
      end
    end

    # TODO: move this to specs dir
    def run_tests(
      test_size:  Integer(ENV.fetch("TEST_SIZE",  1000)),
      test_count: Integer(ENV.fetch("TEST_COUNT", 1000)),
      io: $stdout
    )
      io.puts "Testing all implementations. . ."
      refill_random_vals

      test_count.times do
        IMPLEMENTATIONS.each do |impl|
          queue = impl.klass.new
          values = Array.new(test_size) {
            @dheap_bm_random_vals.pop or raise OutOfRandomness
          }
          values.each do |v| queue << v end
          popped = []
          while (val = queue.pop)
            popped << val
          end
          next if popped == values.sort

          io.puts "=== BROKEN %s ===" % [impl.name]
          io.puts "  sorted    => %p" % [values.sort]
          io.puts "  popped    => %p" % [popped]
          io.puts "  remainder => %p" % [queue.a]
          raise "Broken %s: %p != %p, %p" % [impl.name, values.sort, popped, queue.a]
        end
      end

      io.puts "Tests OK (count=%d, size=%d)" % [test_count, test_size]
      io.puts
    end

    # rubocop:enable Metrics/MethodLength, Metrics/AbcSize, Style/MultilineBlockChain

  end
end
