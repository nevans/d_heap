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

    def add_warmup_attrs(value, duration, loop_count)
      self.warmup_value = value
      self.warmup_duration = duration
      self.warmup_loop_count = loop_count
    end

  end

  # BenchmarkDriver::Runner looks for this class
  JobParser = BenchmarkDriver::DefaultJobParser.for(klass: Job, metrics: [METRIC])

  # This method is dynamically called by `BenchmarkDriver::JobRunner.run`
  # @param [Array<BenchmarkDriver::Default::Job>] jobs
  def run(jobs)
    jobs = run_all_jobs_warmup(jobs)
    run_all_jobs_benchmarks(jobs)
  end

  def run_all_jobs_warmup(jobs)
    return jobs if jobs.all?(&:loop_count)
    @output.with_warmup do
      jobs.map! {|job|
        # skip warmup if loop_count is set
        job.loop_count ? job : output_warmup_and_config_job(job)
      }
    end
  end

  def run_all_jobs_benchmarks(jobs)
    @output.with_benchmark do
      jobs.each do |job|
        @output.with_job(name: job.name) do
          job.runnable_contexts(@contexts).each do |context|
            run_and_report_job(job, context)
          end
        end
      end
    end
  end

  def output_warmup_and_config_job(job)
    @output.with_job(name: job.name) do
      context = job.runnable_contexts(@contexts).first
      value, duration, warmup_loop_count = run_and_report_warmup_job(job, context)
      loop_count = (warmup_loop_count.to_f * @config.run_duration / duration).floor
      Job.new(**job.to_h.merge(loop_count: loop_count))
        .tap {|j| j.add_warmup_attrs(value, duration, warmup_loop_count) }
    end
  end

  def run_and_report_warmup_job(job, context)
    duration, loop_count = run_warmup(job, context: context)
    value, duration = value_duration(duration: duration, loop_count: loop_count)
    output_with_context(context) do
      @output.report(
        values: {metric => value}, duration: duration, loop_count: loop_count
      )
    end
    [value, duration, loop_count]
  end

  def run_and_report_job(job, context)
    result, loop_count = run_job_with_repeater(job, context)
    value, duration = result.value
    output_with_context(context) do
      @output.report(
        values: { metric => value },
        all_values: { metric => result.all_values },
        duration: duration,
        loop_count: loop_count,
      )
    end
  end

  def output_with_context(context, &block)
    @output.with_context(
      name: context.name,
      executable: context.executable,
      gems: context.gems,
      prelude: context.prelude,
      &block
    )
  end

  def run_job_with_repeater(job, context)
    repeat_params = { config: @config, larger_better: true, rest_on_average: :average }
    if job.loop_count&.positive?
      run_job_with_own_loop_count(job, context, repeat_params)
    else
      run_job_with_warmup_loop_count(job, context, repeat_params)
    end
  end

  def run_job_with_own_loop_count(job, context, repeat_params)
    loop_count = job.loop_count
    result = BenchmarkDriver::Repeater.with_repeat(**repeat_params) {
      run_benchmark(job, context: context)
    }
    [result, loop_count]
  end

  def run_job_with_warmup_loop_count(job, context, repeat_params)
    loop_count = job.warmup_loop_count
    repeater_value = [job.warmup_value, job.warmup_duration]
    result = BenchmarkDriver::Repeater::RepeatResult.new(
      value: repeater_value, all_values: [repeater_value]
    )
    [result, loop_count]
  end

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
