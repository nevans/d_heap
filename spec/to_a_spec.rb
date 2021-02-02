# frozen_string_literal: true

return unless defined?(DHeap::Map)

RSpec.describe DHeap::Map do

  describe_any_size_heap "#to_a" do

    it "returns an empty array from an empty heap" do
      expect(heap.to_a).to eq([])
    end

    it "returns [[object, score], ...] in heap order" do
      heap.push("abc", 123.456789);
      expect(heap.to_a).to eq([["abc", 123.456789]])
      heap.push("foo", 37);
      expect(heap.to_a).to eq([["foo", 37], ["abc", 123.456789]])
      heap.push("bar", 42);
      expect(heap.to_a).to eq([["foo", 37], ["abc", 123.456789], ["bar", 42]])
      heap.push("baz", 2301);
      expect(heap.to_a).to eq(
        [["foo", 37], ["abc", 123.456789], ["bar", 42], ["baz", 2301]]
      )
      expect(heap.to_a)
    end

  end

end
