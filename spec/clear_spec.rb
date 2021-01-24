# frozen_string_literal: true

RSpec.describe DHeap do
  describe_any_size_heap "#clear" do

    it "returns self" do
      expect(heap.clear).to eq(heap)
    end

    it "resets size to zero" do
      heap.clear
      expect(heap.size).to eq(0)
    end

    it "clears the heap so peek returns nil" do
      heap.clear
      expect(heap.peek).to be_nil
    end

  end
end
