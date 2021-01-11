# frozen_string_literal: true

require "d_heap/benchmarks"

require "ruby-prof"

module DHeap::Benchmarks
  # Profiles different implementations with different sizes
  class Profiler
    include Randomness
    include Scenarios

    N_COUNTS = [
      5,      # 1 + 4
      1365,   # 1 + 4 + 16 + 64 + 256 + 1024
      87_381, # 1 + 4 + 16 + 64 + 256 + 1024 + 4096 + 16384 + 65536
    ].freeze

    def call(
      queue_size: ENV.fetch("PROFILE_QUEUE_SIZE", :unset),
      iterations: ENV.fetch("PROFILE_ITERATIONS", 1_000_000)
    )
      DHeap::Benchmarks.puts_version_info("Profiling")
      fill_random_vals
      sizes = queue_size == :unset ? N_COUNTS : [Integer(queue_size)]
      sizes.each do |size|
        profile_all(size, iterations)
      end
    end

    def profile_all(queue_size, iterations, io: $stdout)
      io.puts <<~TEXT
        ########################################################################
        # Profile w/ N=#{queue_size} (i=#{iterations})
        # (n.b. RubyProf & tracepoint can change relative performance.
        #       A sampling profiler can provide more accurate relative metrics.
        ########################################################################

      TEXT
      DHeap::Benchmarks::IMPLEMENTATIONS.each do |impl|
        profile_one(impl, queue_size, iterations, io: io)
      end
    end

    # TODO: move somewhere else...
    def skip_profiling?(queue_size, impl)
      impl.klass == DHeap::Benchmarks::Sorting && 10_000 < queue_size
    end

    def profile_one(impl, queue_size, iterations, io: $stdout)
      return if skip_profiling?(queue_size, impl)
      io.puts "Filling   #{impl.name} ---------------------------"
      queue = impl.klass.new
      push_n(queue, queue_size)
      io.puts "Profiling #{impl.name} ---------------------------"
      profiling do
        repeated_push_pop(queue, iterations)
      end
    end

    def profiling(io: $stdout, &block)
      # do the thing
      result = RubyProf.profile(&block)
      # report_the_thing
      printer = RubyProf::FlatPrinter.new(result)
      printer.print($stdout, min_percent: 1.0)
      io.puts
    end

  end
end
