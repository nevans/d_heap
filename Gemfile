# frozen_string_literal: true

source "https://rubygems.org"

# Specify your gem's dependencies in d_heap.gemspec
gemspec

gem "pry"
gem "rake", "~> 13.0"
gem "rake-compiler"
gem "rspec", "~> 3.10"
gem "rubocop", "~> 1.0"

gem "fuubar"

install_if -> { RUBY_PLATFORM !~ /darwin/ } do
  gem "benchmark_driver-output-gruff"
end

gem "perf"
gem "priority_queue_cxx"
gem "stackprof"
