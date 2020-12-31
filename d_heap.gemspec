# frozen_string_literal: true

require_relative "lib/d_heap/version"

Gem::Specification.new do |spec|
  spec.name          = "d_heap"
  spec.version       = DHeap::VERSION
  spec.authors       = ["nicholas a. evans"]
  spec.email         = ["nicholas.evans@gmail.com"]

  spec.summary       = "A d-ary heap implementation, for priority queues"
  spec.description   = <<~DESC
    A C extension implementation of a d-ary heap data structure, suitable for
    use in e.g. priority queues or Djikstra's algorithm.
  DESC
  spec.homepage      = "https://github.com/nevans/#{spec.name}"
  spec.license       = "MIT"
  spec.required_ruby_version = Gem::Requirement.new(">= 2.5.0")

  spec.metadata["homepage_uri"] = spec.homepage
  spec.metadata["source_code_uri"] = spec.homepage
  spec.metadata["changelog_uri"] = "#{spec.homepage}/blob/master/Changelog.md"

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been added into git.
  spec.files = Dir.chdir(File.expand_path(__dir__)) {
    `git ls-files -z`.split("\x0").reject {|f| f.match(%r{^(test|spec|features)/}) }
  }
  spec.bindir        = "exe"
  spec.executables   = spec.files.grep(%r{^exe/}) {|f| File.basename(f) }
  spec.require_paths = ["lib"]
  spec.extensions    = ["ext/d_heap/extconf.rb"]

  spec.add_development_dependency "benchmark-ips"
  spec.add_development_dependency "rspec-benchmark"
  spec.add_development_dependency "ruby-prof"
end
