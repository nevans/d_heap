# frozen_string_literal: true

require "d_heap/benchmarks"

require "benchmark_driver"
require "shellwords"
require "English"

module DHeap::Benchmarks
  # Benchmarks different implementations with different sizes
  class Benchmarker
    include Randomness
    include Scenarios

    N_COUNTS = [
      5,      # 1 + 4
      21,     # 1 + 4 + 16
      85,     # 1 + 4 + 16 + 64
      341,    # 1 + 4 + 16 + 64 + 256
      1365,   # 1 + 4 + 16 + 64 + 256 + 1024
      5461,   # 1 + 4 + 16 + 64 + 256 + 1024 + 4096
      21_845, # 1 + 4 + 16 + 64 + 256 + 1024 + 4096 + 16384
      87_381, # 1 + 4 + 16 + 64 + 256 + 1024 + 4096 + 16384 + 65536
    ].freeze

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
      sizes = (queue_size == :unset) ? N_COUNTS : [Integer(queue_size)]
      sizes.each do |size|
        benchmark_size(size)
      end
    end

    def benchmark_size(size)
      sep "#", "Benchmarks with N=#{size} (t=#{time}sec/benchmark)", big: true
      io.puts
      benchmark_push_n            size
      benchmark_push_n_then_pop_n size
      benchmark_repeated_push_pop size
    end

    def benchmark_push_n(queue_size)
      benchmarking("push N", "push_n", queue_size)
    end

    def benchmark_push_n_then_pop_n(queue_size)
      benchmarking("push N then pop N", "push_n_pop_n", queue_size)
    end

    def benchmark_repeated_push_pop(queue_size)
      benchmarking(
        "Push/pop with pre-filled queue (size=N)", "push_pop", queue_size
      )
    end

    private

    # TODO: move somewhere else...
    def skip_profiling?(queue_size, impl)
      impl.klass == DHeap::Benchmarks::PushAndResort && 10_000 < queue_size
    end

    # rubocop:disable Metrics/MethodLength, Metrics/AbcSize

    def benchmarking(name, file, size)
      Bundler.with_unbundled_env do
        sep "==", "#{name} (N=#{size})"
        cmd = %W[
          bin/benchmark-driver
          --bundler
          --run-duration 6
          --timeout 15
          --runner ips_zero_fail
          benchmarks/#{file}.yml
        ]
        if file == "push_n"
          cmd << "--filter" << /dheap|\bstl\b|\bbsearch\b|\brb_heap\b/.to_s
        end
        env = ENV.to_h.merge(
          "BENCH_N" => size.to_s,
          "RUBYLIB" => File.expand_path("../..", __dir__),
        )
        system(env, *cmd)
      end
    end

    def sep(sep, msg = "", width: 80, big: false)
      txt = String.new
      txt += "#{sep * (width / sep.length)}\n" if big
      txt += sep
      txt += " #{msg}" if msg && !msg.empty?
      txt += " " unless big
      txt += sep * ((width - txt.length) / sep.length) unless big
      txt += "\n"
      txt += "#{sep * (width / sep.length)}\n" if big
      io.print txt
    end

    # rubocop:enable Metrics/MethodLength, Metrics/AbcSize

  end
end
