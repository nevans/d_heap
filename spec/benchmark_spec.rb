# frozen_string_literal: true

require "d_heap/benchmarks/rspec_matchers"

RSpec.describe DHeap, "benchmarks" do
  include DHeap::Benchmarks::Randomness
  include DHeap::Benchmarks::Scenarios
  include DHeap::Benchmarks::RSpecMatchers

  subject(:heap) { DHeap.new }
  subject(:sorted) { DHeap::Benchmarks::BinarySearchAndInsert.new }

  # don't use more than 1 million values per benchmark
  def max_bm_items; 25_000_000 end
  def max_warmup;   2000 end
  def max_prefills; 100_000 end

  # two benchmark, for comparisons
  def default_randomness_size; max_bm_items + max_prefills + max_warmup end

  def silent; ENV["RSPEC_BENCHMARK_VOLUME"] != "loud" end

  # using before(:all) allows the instance var to be re-used between all specs
  before(:all)  do fill_random_vals(io: silent ? nil : $stderr) end
  before(:each) do
    fill_random_vals(io: silent ? nil : $stderr)
    # try to avoid doing these during the test run...
    GC.start(full_mark: true, immediate_sweep: true)
    GC.compact
  end

  context "iterations per second" do
    let(:total_pushes) { 50_000_000 }
    it "pushes and pops on an empty heap at least 5M items/sec" do
      expect do |i|
        repeated_push_pop(heap, i)
      end
        .to perform_at_least(5_000_000).ips
        .running_at_most(25_000_000).times
        .running_at_most(5).seconds
        .iterations_per_round(100_000)
      expect(heap).to be_empty
    end

    it "pushes and pops on an 50k item heap at least 1M items/sec" do
      push_n(heap, 50_000)
      expect do |i|
        repeated_push_pop(heap, i)
      end
        .to perform_at_least(1_000_000).ips
        .running_at_most(10_000_000).times
        .running_at_most(5).seconds
        .iterations_per_round(100_000)
      expect(heap.size).to eq(50_000)
    end

  end

  describe "vs bsearch insert into sorted array" do

    # rubocop:disable Style/BlockDelimiters

    shared_examples "pushes faster" do |count, times|
      let(:items_per_round) { 100 * count }
      let(:max_items) {
        12_500_000 / items_per_round * items_per_round # int division truncation
      }
      it "pushes #{count} items >= #{times}x faster" do
        expect do |i|
          i /= count
          while 0 < i
            heap.clear
            push_n(heap, count)
            i -= 1
          end
        end.to(perform_at_least(times).times_faster_than do |i|
          i /= count
          while 0 < i
            sorted.clear
            push_n(sorted, count)
            i -= 1
          end
        end
          .running_at_most(max_items).times
          .running_at_most(5).seconds
          .warmup_at_most(0).times
          .iterations_per_round(100 * count))
      end
    end

    shared_examples "pushes and pops faster with pre-filled queue" do |count, times|
      it "pushes then pops on existing queue (size=#{count}) #{times}x faster" do
        push_n_multiple_queues(count, heap, sorted)
        expect do |items_per_round|
          repeated_push_pop(heap, items_per_round)
        end.to(perform_at_least(times).times_faster_than do |items_per_round|
          repeated_push_pop(sorted, items_per_round)
        end
          .running_at_most(10_000_000).times
          .running_at_most(5).seconds
          .warmup_at_most(0).times
          .iterations_per_round(10_000))
      end
    end

    # rubocop:enable Style/BlockDelimiters

    describe "with 10 items" do
      include_examples "pushes faster",                                15, 1.5
      include_examples "pushes and pops faster with pre-filled queue", 15, 1.01
    end

    describe "with 100 items" do
      include_examples "pushes faster",                                127, 2.5
      include_examples "pushes and pops faster with pre-filled queue", 127, 1.01
    end

    describe "with 1000 items" do
      include_examples "pushes faster",                                1023, 3
      include_examples "pushes and pops faster with pre-filled queue", 1023, 1.05
    end

    describe "with 10_000 items" do
      include_examples "pushes faster",                                8191, 3.5
      include_examples "pushes and pops faster with pre-filled queue", 8191, 1.05
    end

  end

end
