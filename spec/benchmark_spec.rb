# frozen_string_literal: true

require "rspec-benchmark"

RSpec::Benchmark.configure do |config|
  config.run_in_subprocess = true
  config.samples = 10 # for "perform_under"
end

RSpec.describe DHeap, "benchmarks" do
  include RSpec::Benchmark::Matchers

  subject(:heap) { DHeap.new }

  describe "iterations" do

    it "should perform at least 50k/s pushes and pops on an empty heap" do
      expect do
        heap.push 1, "a"
        expect(heap.pop).to eq("a")
        expect(heap).to be_empty
      end.to perform_at_least(50_000).within(2).ips
    end

    it "should perform at least 50k/s pushes and pops on an 10k item heap" do
      10_000.times do heap.push rand(0..100_000), "val" end
      expect do
        heap.push rand(0..100_000), "val"
        heap.pop
        expect(heap.size).to eq(10_000)
      end.to perform_at_least(50_000).within(2).ips
    end

  end

  describe "vs bsearch insert into sorted array" do

    # not exactly the same API, but close enough
    let(:sorted_array) do
      Class.new do

        def initialize
          @a = []
        end

        def <<(score)
          index = @a.bsearch_index {|other| score > other } || @a.length
          @a.insert(index, score)
        end

        def pop
          @a.pop
        end

        def empty?
          @a.empty?
        end

      end.new
    end

    shared_examples "inserts faster" do |count, times|
      it "inserts #{count} items #{times}x faster" do
        expect do
          count.times do heap << rand(1..100) end
        end.to(perform_faster_than do # rubocop:disable Style/BlockDelimiters
          count.times do sorted_array << rand(1..100) end
        end.at_least(times).within(2))
      end
    end

    # unfortunately, min-delete is the *slowest* operation for a heap
    shared_examples "pops faster" do |count|
      it "pops #{count} items faster" do
        count.times do heap << rand(1..100) end
        count.times do sorted_array << rand(1..100) end
        expect do
          heap.pop until heap.empty?
        end.to(perform_faster_than do # rubocop:disable Style/BlockDelimiters
          sorted_array.pop until sorted_array.empty?
        end)
      end
    end

    shared_examples "inserts and pops faster" do |count|
      it "inserts and deletes #{count} items faster" do
        count.times do heap << rand(1..100) end
        count.times do sorted_array << rand(1..100) end
        expect do
          heap << rand(1..100)
          heap.pop
        end.to(perform_faster_than do # rubocop:disable Style/BlockDelimiters
          sorted_array << rand(1..100)
          sorted_array.pop
        end.within(2))
      end
    end

    # describe "with 3 items" do
    #   include_examples "inserts faster",          3, 10
    #   # include_examples "pops faster",             3
    #   include_examples "inserts and pops faster", 3
    # end

    describe "with 10 items" do
      include_examples "inserts faster",          10, 10
      # include_examples "pops faster",             10
      include_examples "inserts and pops faster", 10
    end

    describe "with 100 items" do
      include_examples "inserts faster",          100, 10
      # include_examples "pops faster",             100
      include_examples "inserts and pops faster", 100
    end

    describe "with 1000 items" do
      include_examples "inserts faster",          1000, 10
      # include_examples "pops faster",             1000
      include_examples "inserts and pops faster", 1000
    end

    describe "with 10_000 items" do
      include_examples "inserts faster",          10_000, 15
      # include_examples "pops faster",             10_000
      include_examples "inserts and pops faster", 10_000
    end

  end

end
