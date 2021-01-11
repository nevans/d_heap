# frozen_string_literal: true

require "English" # $CHILD_STATUS
require "timeout" # Timeout::Error

require "benchmark_driver"

# monkey-patch to convert miniscule values to 0.0
class BenchmarkDriver::Output::Compare

  # monkey-patch to convert miniscule values to 0.0
  module MinisculeToZero

    def humanize(value, width = 10)
      value <= 0.0.next_float.next_float ? 0.0 : super(value, width)
    end

  end

  prepend MinisculeToZero

end

# A simple patch to let slow specs error out without
class BenchmarkDriver::Runner::IpsZeroFail < BenchmarkDriver::Runner::Ips
  METRIC = BenchmarkDriver::Runner::Ips::METRIC

  # always run at least once
  class Job < BenchmarkDriver::DefaultJob
    attr_accessor :warmup_value, :warmup_duration, :warmup_loop_count

  end

  # BenchmarkDriver::Runner looks for this class
  JobParser = BenchmarkDriver::DefaultJobParser.for(klass: Job, metrics: [METRIC])

  # rubocop:disable Metrics/MethodLength, Metrics/AbcSize, Metrics/PerceivedComplexity, Metrics/BlockLength, Layout/LineLength, Layout/SpaceInsideBlockBraces, Style/BlockDelimiters

  # This method is dynamically called by `BenchmarkDriver::JobRunner.run`
  # @param [Array<BenchmarkDriver::Default::Job>] jobs
  def run(jobs)
    if jobs.any? { |job| job.loop_count.nil? }
      @output.with_warmup do
        jobs = jobs.map do |job|
          next job if job.loop_count # skip warmup if loop_count is set

          @output.with_job(name: job.name) do
            context = job.runnable_contexts(@contexts).first
            duration, loop_count = run_warmup(job, context: context)
            value, duration = value_duration(duration: duration, loop_count: loop_count)

            @output.with_context(name: context.name, executable: context.executable, gems: context.gems, prelude: context.prelude) do
              @output.report(values: { metric => value }, duration: duration, loop_count: loop_count)
            end

            warmup_loop_count = loop_count

            loop_count = (loop_count.to_f * @config.run_duration / duration).floor
            Job.new(**job.to_h.merge(loop_count: loop_count))
              .tap {|j| j.warmup_value      = value }
              .tap {|j| j.warmup_duration   = duration }
              .tap {|j| j.warmup_loop_count = warmup_loop_count }
          end
        end
          .compact
      end
    end

    @output.with_benchmark do
      jobs.each do |job|
        @output.with_job(name: job.name) do
          job.runnable_contexts(@contexts).each do |context|
            repeat_params = { config: @config, larger_better: true, rest_on_average: :average }
            result =
              if job.loop_count&.positive?
                loop_count = job.loop_count
                BenchmarkDriver::Repeater.with_repeat(**repeat_params) do
                  run_benchmark(job, context: context)
                end
              else
                loop_count = job.warmup_loop_count
                repeater_value = [job.warmup_value, job.warmup_duration]
                BenchmarkDriver::Repeater::RepeatResult.new(
                  value: repeater_value, all_values: [repeater_value]
                )
              end
            value, duration = result.value
            @output.with_context(name: context.name, executable: context.executable, gems: context.gems, prelude: context.prelude) do
              @output.report(
                values: { metric => value },
                all_values: { metric => result.all_values },
                duration: duration,
                loop_count: loop_count,
              )
            end
          end
        end
      end
    end
  end

  # rubocop:enable Metrics/MethodLength, Metrics/AbcSize, Metrics/PerceivedComplexity, Metrics/BlockLength, Layout/LineLength, Layout/SpaceInsideBlockBraces, Style/BlockDelimiters

  def run_warmup(job, context:)
    start = Time.now
    super(job, context: context)
  rescue Timeout::Error
    [Time.now - start, 0.0.next_float]
  end

  def execute(*args, exception: true)
    super
  rescue RuntimeError => ex
    if args.include?("timeout") && $CHILD_STATUS&.exitstatus == 124
      raise Timeout::Error, ex.message
    end
    raise ex
  end

end
