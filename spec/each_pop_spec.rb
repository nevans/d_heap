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

    it "can also sends the scores" do
      a = []
      b = []
      heap.clear
      heap.push(obj1 = Object.new, 1);
      heap.push(obj2 = Object.new, 2);
      heap.push(obj3 = Object.new, 3);
      heap.each_pop(with_scores: true) do |val, score| a << val; b << score end
      expect(a).to eq([obj1, obj2, obj3])
      expect(b).to eq([1, 2, 3])
      expect(heap).to be_empty
    end

  end
end
