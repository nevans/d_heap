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

    # approximation for multiples of sqrt(10)
    PUSH_POP_HEAP_SIZES = [
      10,
      32,
      100,
      316,
      1000,
      3162,
      10_000,
      31_623,
      100_000,
      316_228,
      1_000_000,
      3_162_278,
      10_000_000,
    ].freeze

    # slightly off approximation for multiples of sqrt(10)
    # hard-coded "loop_count" (in yml file) must be cleanly divisible by N.
    PUSH_N_COUNTS = [
      10,
      30,
      100,
      300,
      1000,
      3000,
      1_000,
      3_000,
      10_000,
      30_000,
      100_000,
      300_000,
      1_000_000,
      3_000_000,
    ].freeze

    COMBO_SIZES = (PUSH_N_COUNTS + PUSH_POP_HEAP_SIZES).sort.uniq.freeze

    attr_reader :count
    attr_reader :iterations_for_push_pop
    attr_reader :io

    def initialize(
      count: Integer(ENV.fetch("BENCHMARK_REPEATS", 4)),
      iterations_for_push_pop: 10_000,
      io: $stdout
    )
      @count = count
      @iterations_for_push_pop = Integer(iterations_for_push_pop)
      @io = io
    end

    def call
      DHeap::Benchmarks.puts_version_info("Benchmarking")
      COMBO_SIZES.each do |size|
        benchmarks_for_n(size)
      end
    end

    def benchmarks_for_n(size)
      sep_big "#", "Benchmarks with N=#{format_num size}"
      if PUSH_N_COUNTS.include?(size)
        benchmark_push_n            size
        benchmark_push_n_then_pop_n size
      end
      if PUSH_POP_HEAP_SIZES.include?(size)
        benchmark_repeated_push_pop size
      end
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

    def benchmarking(name, file, size)
      Bundler.with_unbundled_env do
        sep "==", "#{name} (N=#{size})"
        cmd = benchmark_cmd(file, size)
        env = ENV.to_h.merge(
          "BENCH_N" => size.to_s,
          "RUBYLIB" => File.expand_path("../..", __dir__),
        )
        system(env, *cmd)
      end
    end

    def benchmark_cmd(file, size)
      cmd = %W[bin/benchmark-driver --bundler --repeat-count #{count}]
      add_filter(file, size, cmd)
      cmd << "benchmarks/#{file}.yml"
    end

    def add_filter(file, size, cmd)
      case file
      when "push_n"
        add_push_n_filter(size, cmd)
      when "push_n_pop_n"
        add_push_n_pop_n_filter(size, cmd)
      end
    end

    def format_num(number)
      number.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse
    end

    def add_push_n_filter(size, cmd)
      if 300_000 <= size
        cmd << "--filter" << /dheap|stl|rb_heap|findmin/.to_s
      end
    end

    def add_push_n_pop_n_filter(size, cmd)
      if 300_000 <= size
        cmd << "--filter" << /dheap|stl|rb_heap/.to_s
      elsif 3_000 <= size
        cmd << "--filter" << /dheap|stl|rb_heap|bsearch/.to_s
      end
    end

    def sep(sepstr, msg = "", width: 80)
      txt = String.new
      txt += sepstr
      txt += " #{msg} " if msg && !msg.empty?
      txt += sepstr * ((width - txt.length) / sepstr.length)
      io.puts txt
    end

    def sep_big(sepstr, msg = "", width: 80)
      txt = String.new
      txt += "#{sepstr * (width / sepstr.length)}\n"
      txt += sepstr
      txt += " #{msg}" if msg && !msg.empty?
      txt += "\n#{sepstr * (width / sepstr.length)}\n"
      io.puts txt
      io.puts
    end

  end
end
