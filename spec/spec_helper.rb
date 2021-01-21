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
