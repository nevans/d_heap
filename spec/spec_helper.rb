# frozen_string_literal: true

require "bundler/setup"
require "d_heap"

RSpec.configure do |config|
  # Enable flags like --only-failures and --next-failure
  config.example_status_persistence_file_path = ".rspec_status"

  # Disable RSpec exposing methods globally on `Module` and `main`
  config.disable_monkey_patching!

  config.expect_with :rspec do |c|
    c.syntax = :expect
  end
end

# simplifies spec compatibility with ruby 2.4
FrozenError = RuntimeError unless defined?(FrozenError)

module SpecHelper

  def describe_any_size_heap(name, &block) # rubocop:disable Metrics/MethodLength
    describe name do

      shared_examples(name) do |dval|
        let(:d) { dval }
        subject(:heap) { DHeap.new(d: d) }
        instance_exec(dval, &block)
      end

      [2, 3, 4, 5, 8, 16].each do |d|
        context "heap with d=#{d}" do
          include_examples name, d
        end
      end

    end
  end

end

RSpec.configure do |config|
  config.extend SpecHelper
end
