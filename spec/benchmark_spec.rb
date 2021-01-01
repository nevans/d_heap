# frozen_string_literal: true

require "rspec-benchmark"

RSpec.describe DHeap, "benchmarks" do
  include RSpec::Benchmark::Matchers

  subject(:heap) { DHeap.new }
  let(:seconds_to_run) { 5 }

  describe "iterations" do

    it "should perform at least 50k/s pushes and pops on an empty heap" do
      expect do
        heap.push 1, "a"
        expect(heap.pop).to eq("a")
        expect(heap).to be_empty
      end.to perform_at_least(50_000).within(seconds_to_run).ips
    end

    it "should perform at least 50k/s pushes and pops on an 10k item heap" do
      10_000.times do heap.push rand(0..100_000), "val" end
      expect do
        heap.push rand(0..100_000), "val"
        heap.pop
        expect(heap.size).to eq(10_000)
      end.to perform_at_least(50_000).within(seconds_to_run).ips
    end

  end

  describe "vs bsearch insert into sorted array" do

    # not exactly the same API, but close enough
    let(:sorted) do
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

    # rubocop:disable Style/BlockDelimiters

    shared_examples "pushes faster" do |count, times|
      let!(:random_vals) {
        # removes a source of slowdown and variation from the measured perf
        Array.new(4_000_000 * seconds_to_run) { rand(1..10_000) }
      }
      it "pushes #{count} items >= #{times}x faster" do
        expect do
          count.times do heap << random_vals.pop end
        end.to(perform_faster_than do
          count.times do sorted << random_vals.pop end
        end.at_least(times).within(seconds_to_run))
      end
    end

    shared_examples "pushes and pops faster with pre-filled queue" do |count|
      let(:items_per_round) { 10_000 }
      let!(:random_vals) {
        # removes a source of slowdown and variation from the measured perf
        Array.new(4_000_000 * seconds_to_run) { rand(1..100_000) }
      }
      it "pushes then pops on existing queue (size=#{count}) faster" do
        count.times do
          val = random_vals.pop
          heap << val
          sorted << val
        end
        expect do
          i = items_per_round # to reduce block overhead
          while 0 < i
            heap << random_vals.pop or raise "out of values"
            heap.pop
            i -= 1
          end
        end.to(perform_faster_than do
          i = items_per_round # to reduce block overhead
          while 0 < i
            sorted << random_vals.pop or raise "out of values"
            sorted.pop
            i -= 1
          end
        end.within(seconds_to_run))
      end
    end

    # rubocop:enable Style/BlockDelimiters

    describe "with 10 items" do
      include_examples "pushes faster",                                10, 10
      include_examples "pushes and pops faster with pre-filled queue", 10
    end

    describe "with 100 items" do
      include_examples "pushes faster",                                100, 10
      include_examples "pushes and pops faster with pre-filled queue", 100
    end

    describe "with 1000 items" do
      include_examples "pushes faster",                                1000, 10
      include_examples "pushes and pops faster with pre-filled queue", 1000
    end

    describe "with 10_000 items" do
      include_examples "pushes faster",                                10_000, 15
      include_examples "pushes and pops faster with pre-filled queue", 10_000
    end

  end

end
