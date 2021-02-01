# frozen_string_literal: true

require "bundler/gem_tasks"
require "rake/extensiontask"
require "rspec/core/rake_task"
require "rubocop/rake_task"

GEMSPEC = Gem::Specification.load("d_heap.gemspec")

desc "Run RSpec code examples, without benchmarks"
RSpec::Core::RakeTask.new(:spec) do |t|
  t.rspec_opts = "--tag ~benchmarks"
end

desc "Run all RSpec code examples, including benchmarks"
RSpec::Core::RakeTask.new("spec:benchmarks")

RuboCop::RakeTask.new

task build: :compile

Rake::ExtensionTask.new("d_heap", GEMSPEC) do |ext|
  ext.lib_dir = "lib/d_heap"
end

desc "Enable debug compilation options. Might hurt performance."
task "extconf:debug" do
  ENV["EXTCONF_DEBUG"] = "1"
end

task default: %i[clobber compile spec rubocop]

task ci: %i[clobber extconf:debug compile spec rubocop]
task "ci:benchmarks" => %i[clobber compile spec:benchmarks]
