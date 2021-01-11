# frozen_string_literal: true

require "d_heap/benchmarks"

module DHeap::Benchmarks

  # Profiles different implementations with different sizes
  module RSpecMatchers # rubocop:disable Metrics/ModuleLength
    extend RSpec::Matchers::DSL

    # Assert ips (iterations per second):
    #
    #     expect { ... }.to perform_at_least(1_000_000).ips
    #        .running_at_least(10).times             # optional, defaults to  1
    #        .running_at_least(10).seconds           # optional, defaults to  1s
    #        .running_at_most(10_000_000).times      # optional, defaults to nil
    #        .running_at_most(2).seconds             # optional, defaults to  2s
    #        .warmup_at_most(1000).times             # optional, defaults to  1k
    #        .warmup_at_most(0.100).seconds          # optional, defaults to  0.1s
    #        .iterations_per_round                   # optional, defaults to  1
    #        .and_at_least(1.1).times.faster_than { ... } # can also compare
    #
    # Assert comparison (and optionally runtime or ips):
    #
    #     expect { ... }.to perform_at_least(2.5).times_faster_than { ... }
    #        .running_at_least(10).times             # optional, defaults to  1
    #        .running_at_least(10).seconds           # optional, defaults to  1s
    #        .running_at_most(10_000_000).times      # optional, defaults to nil
    #        .running_at_most(2).seconds             # optional, defaults to  2s
    #        .warmup_at_most(1000).times             # optional, defaults to  1k
    #        .warmup_at_most(0.100).seconds          # optional, defaults to  0.1s
    #        .iterations_per_call                    # optional, defaults to  1
    #        .and_at_least(100).ips { ... } # can also assert ips
    #
    # n.b: Given a known constant number of iterations, run time and ips are both
    # measuring the same underlying metric.
    #
    # rubocop:disable Metrics/BlockLength, Layout/SpaceAroundOperators
    matcher :perform_at_least do |expected|
      supports_block_expectations

      def __debug__(name, caller_binding)
        lvars = __debug_lvars__(caller_binding)
        ivars = __debug_ivars__(caller_binding)
        puts "%s, locals => %p, ivars => %p" % [name, lvars, ivars]
      end

      def __debug_lvars__(caller_binding)
        caller_binding.local_variables.map {|lvar|
          next if %i[type unit].include?(lvar)
          next if (val = caller_binding.local_variable_get(lvar)).nil?
          [lvar, val]
        }.compact.to_h
      end

      def __debug_ivars__(caller_binding)
        instance_variables.map {|ivar|
          next if %i[@name @actual @expected_as_array @matcher_execution_context
                     @chained_method_clauses @block_arg]
            .include?(ivar)
          next if (val = instance_variable_get(ivar)).nil?
          [ivar, val]
        }.compact.to_h
      end

      %i[
        is_at_least
        running_at_most
        running_at_least
        warmup_at_most
      ].each do |type|
        chain type do |number|
          # __debug__ "%s(%p)" % [type, number], binding
          reason, value = ___number_reason_and_value___
          if reason || value
            raise "Need to handle unit-less number first: %s(%p)" % [reason, value]
          end
          @number_for = type
          @number_val = number
        end
      end

      alias_method :and_at_least, :is_at_least

      %i[
        times
        seconds
        milliseconds
      ].each do |unit|
        chain unit do
          # __debug__ unit, binding
          reason, value = ___number_reason_and_value___
          raise "No number was specified" unless reason && value
          case reason
          when :running_at_most;  apply_max_run unit
          when :running_at_least; apply_min_run unit
          when :warmup_at_most;   apply_warmup  unit
          else raise "%s is incompatible with %s(%p)" % [unit, reason, value]
          end
          @number_for = @number_val = nil
        end
      end

      # TODO: let IPS set time to run instead of iterations to run
      chain :ips do
        # __debug__ "ips", binding
        reason, value = ___number_reason_and_value___
        raise "'ips' unit is only for assertions" unless reason == :is_at_least
        raise "Already asserting %s ips" % [@expect_ips] if @expect_ips
        raise "'ips' assertion has already been made" if @expect_ips
        raise "Unknown assertion count" unless value
        @expect_ips = Integer(value)
        @number_for = @number_val = nil
      end

      # need to use method because "chain" can't take a block
      def times_faster_than(&other)
        # __debug__ "times_faster_than"
        reason, value = ___number_reason_and_value___
        raise "'times_faster_than' is only for assertions" unless reason == :is_at_least
        raise "Already asserting %sx comparison" % [@expect_cmp] if @expect_cmp
        raise ArgumentError, "must provide a proc" unless other
        @expect_cmp = Float(value)
        @cmp_proc = other
        @number_for = @number_val = nil
        self
      end

      chain :loudly  do @volume = :loud  end
      chain :quietly do @volume = :quiet end
      chain :volume do |volume|
        raise "Invalid volume" unless %i[loud quiet].include?(volume)
        @volume = volume
      end

      chain :iterations_per_round do |iterations|
        if @iterations_per_round
          raise "Already set iterations per round (%p)" [@iterations_per_round]
        end
        @iterations_per_round = Integer(iterations)
      end

      match do |actual|
        require "benchmark"
        raise "Need to expect a proc or block" unless actual.respond_to?(:to_proc)
        raise "Need a performance assertion" unless assertion?
        @actual_proc = actual
        prepare_for_measurement
        if @max_iter && (@max_iter % @iterations_per_round) != 0
          raise "Iterations per round (%p) must divide evenly by max iterations (%p)" % [
            @iterations_per_round, @max_iter,
          ]
        end
        run_measurements
        cmp_okay? && ips_okay?
      end

      description do
        [
          @expect_cmp && cmp_okay_msg,
          @expect_ips && ips_okay_msg,
        ].join(", and ")
      end

      failure_message do
        [
          cmp_okay? ? nil : "expected to #{cmp_okay_msg} but #{cmp_fail_msg}",  # =>
          ips_okay? ? nil : "expected to #{ips_okay_msg} but #{ips_fail_msg}",
        ].compact.join(", and ")
      end

      private

      chain :__convert_expected_to_ivars__ do
        @number_val ||= expected
        @number_for ||= :is_at_least if @number_val
        # __debug__ "__convert_expected_to_ivars__", binding
        expected = nil
      end
      private :__convert_expected_to_ivars__

      def ___number_reason_and_value___
        __convert_expected_to_ivars__
        [@number_for, @number_val]
      end

      def apply_min_run(unit)
        case unit
        when :seconds;      @min_time = Float(@number_val)
        when :milliseconds; @min_time = Float(@number_val) / 1000.0
        when :times;        @min_iter = Integer(@number_val)
        else raise "Invalid unit %s for %s(%p)" % [unit, @number_for, @number_val]
        end
      end

      def apply_max_run(unit)
        case unit
        when :seconds;      @max_time = Float(@number_val)
        when :milliseconds; @max_time = Float(@number_val) / 1000.0
        when :times;        @max_iter = Integer(@number_val)
        else raise "Invalid unit %s for %s(%p)" % [unit, @number_for, @number_val]
        end
      end

      def apply_warmup(unit)
        case unit
        when :seconds;      @warmup_time = Float(@number_val)
        when :milliseconds; @warmup_time = Float(@number_val) / 1000.0
        when :times;        @warmup_iter = Integer(@number_val)
        else raise "Invalid unit %s for %s(%p)" % [unit, @number_for, @number_val]
        end
      end

      def prepare_for_measurement
        @volume               ||= ENV.fetch("RSPEC_BENCHMARK_VOLUME", :quiet).to_sym
        @max_time             ||= 2
        @min_time             ||= 1
        @min_iter             ||= 1
        @warmup_time          ||= 0.100
        @warmup_iter          ||= 1000
        @iterations_per_round ||= 1
        nil
      end

      def run_measurements
        puts header if loud?
        # __debug__ "run_measurements", binding
        warmup
        take_measurements
      end

      def header
        max_rounds = @max_iter && @max_iter / @iterations_per_round
        [
          "Warmup time %s, or iterations: %s" % [@min_iter, @max_iter],
          "Benchmark time (%s..%s) or iterations (%s..%s), max rounds: %p" % [
            @min_time, @max_time, @min_iter, @max_iter, max_rounds,
          ],
          "%-10s %s" % ["", Benchmark::CAPTION],
        ].join("\n")
      end

      def warmup
        return unless 0 < @warmup_time && 0 < @warmup_iter # rubocop:disable Style/NumericPredicate
        args = [@warmup_iter, 0, @warmup_time, 1, @warmup_iter]
        measure("warmup",     *args, &@actual_proc)
        measure("warmup cmp", *args, &@cmp_proc) if @cmp_proc
      end

      def take_measurements
        args = [@iterations_per_round, @min_time, @max_time, @min_iter, @max_iter]
        @actual_tms = measure("actual", *args, &@actual_proc)
        @cmp_tms    = measure("cmp",    *args, &@cmp_proc) if @cmp_proc
        return unless @cmp_proc
        @cmp_ratios = @cmp_tms.tms / @actual_tms.tms # how many times faster?
        @actual_cmp = @cmp_ratios.real
        puts "Ran %0.3fx as fast as comparison" % [@actual_cmp] if loud?
      end

      def loud?; @volume == :loud end

      def assertion?; !!(@expect_cmp || @expect_ips) end

      def cmp_okay?; !@expect_cmp || @expect_cmp < @actual_cmp end
      def ips_okay?; !@expect_tms || @expect_tms.ips < @actual_tms.ips end

      def measure(name, ipr, *args)
        measurements = TmsMeasurements.new(name, ipr, *args)
        measurements.max_rounds.times do
          # GC.start(full_mark: true, immediate_sweep: true)
          # GC.compact
          measurements << Benchmark.measure do
            yield ipr
          end
          # p measurements.real
          break if measurements.max_time < measurements.real
        end
        log_measurement(name, measurements)
        measurements
      end

      # rubocop:disable Metrics/AbcSize
      def units_str(num)
        if    num >= 10**12; "%7.3fT" % [num.to_f / 10**12]
        elsif num >= 10** 9; "%7.3fB" % [num.to_f / 10** 9]
        elsif num >= 10** 6; "%7.3fM" % [num.to_f / 10** 6]
        elsif num >= 10** 3; "%7.3fk" % [num.to_f / 10** 3]
        else                 "%7.3f" % [num.to_f]
        end
      end
      # rubocop:enable Metrics/AbcSize

      def log_measurement(name, measurements)
        return unless loud?
        puts "%-10s %s => %s ips (%d rounds)" % [
          name,
          measurements.tms.to_s.rstrip,
          units_str(measurements.ips_real),
          measurements.size,
        ]
      end

      def cmp_okay_msg; "run %0.2fx faster"          % [@expect_cmp] end
      def cmp_fail_msg; "was only %0.2fx as fast"    % [@actual_cmp] end
      def ips_okay_msg; "run with %s ips"            % [units_str(@expect_ips)] end
      def ips_fail_msg; "was only %s ips"            % [units_str(@actual_ips)] end

    end
    # rubocop:enable Metrics/BlockLength, Layout/SpaceAroundOperators

    alias_matcher :perform_with, :perform

  end

  # Replicates a subset of the functionality in benchmark-ips
  #
  # TODO: merge this with benchmark-ips
  # TODO: implement (or remove) min_time, min_iter
  class TmsMeasurements
    attr_reader :iterations_per_entry
    attr_reader :iterations

    attr_reader :min_time
    attr_reader :max_time

    attr_reader :min_iter
    attr_reader :max_iter

    def initialize(name, ipe, min_time, max_time, min_iter, max_iter) # rubocop:disable Metrics/ParameterLists
      @name = name
      @iterations_per_entry = Integer(ipe)
      @min_time = Float(min_time)
      @max_time = Float(max_time)
      @min_iter = Integer(min_iter)
      @max_iter = Integer(max_iter)
      @entries = []
      @sum = Benchmark::Tms.new
      @iterations = 0
    end

    def size; entries.size end

    def <<(tms)
      raise TypeError, "not a #{Benchmark::Tms}" unless tms.is_a?(Benchmark::Tms)
      raise IndexError, "full" if @max_iter <= size
      @sum += tms
      @iterations += @iterations_per_entry
      @entries << tms
      self
    end

    def sum; @sum.dup end
    alias tms sum

    def entries; @entries.dup end

    def cstime; @sum.cstime end
    def cutime; @sum.cutime end
    def real;   @sum.real   end
    def stime;  @sum.stime  end
    def total;  @sum.total  end
    def utime;  @sum.utime  end

    def ips_real;  @iterations / real  end
    def ips_total; @iterations / total end
    def ips_utime; @iterations / utime end

    def max_rounds
      @max_iter && @max_iter / @iterations_per_entry
    end

  end

end
