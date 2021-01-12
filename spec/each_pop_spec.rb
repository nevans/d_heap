# frozen_string_literal: true

RSpec.describe DHeap do
  describe "#each_pop" do

    subject(:heap) { DHeap.new << 3 << 2 << 1 }

    it "iterates through the heap in order" do
      a = []
      heap.each_pop do |val| a << val end
      expect(a).to eq([1, 2, 3])
    end

    it "consumes the heap" do
      heap.each_pop do end
      expect(heap).to be_empty
    end

    it "can be called with a dup to avoid consuming the heap" do
      a = []
      heap.dup.each_pop do |val| a << val end
      expect(a).to eq([1, 2, 3])
      expect(heap.size).to eq(3)
    end

  end
end
