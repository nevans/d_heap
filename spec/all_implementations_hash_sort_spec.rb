# frozen_string_literal: true

require "d_heap/benchmarks/rspec_matchers"

RSpec.describe DHeap, "with every example priority queue implementation" do

  describe "test comparison implementations" do

    def random_val; rand(0..1_000) end

    subject(:pqueue) { described_class.new }

    let(:test_size)  { 4_000 }
    let(:unsorted) { Array.new(test_size) { random_val } }
    let(:sorted) { unsorted.sort }

    DHeap::Benchmarks::IMPLEMENTATIONS.each do |impl|
      describe impl.klass, "(#{impl.name})" do
        it "correctly heap sorts many random values" do
          unsorted.each do |v| pqueue << v end
          popped = []
          while (val = pqueue.pop)
            popped << val
          end
          expect(popped).to eq(sorted)
        end
      end
    end

  end

end
