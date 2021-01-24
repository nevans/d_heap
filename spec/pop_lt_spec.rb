# frozen_string_literal: true

RSpec.describe DHeap do

  describe_any_size_heap "#pop_lte(n)" do
    it "pops only the elements that are compare as <= n" do
      (0..20).to_a.shuffle.each do |i| heap.insert i, {i: i} end
      expect(heap.pop_lte(-1)).to eq(nil)
      expect(heap.pop_lte(4)).to eq({i: 0})
      expect(heap.pop_lte(4)).to eq({i: 1})
      expect(heap.pop_lte(4)).to eq({i: 2})
      expect(heap.pop_lte(4)).to eq({i: 3})
      expect(heap.pop_lte(4)).to eq({i: 4})
      expect(heap.pop_lte(4)).to eq(nil)
    end
  end

  describe_any_size_heap "#pop_lt(n)" do
    it "pops only the elements that are compare as < n" do
      (0..20).to_a.shuffle.each do |i| heap.insert i, {i: i} end
      expect(heap.pop_lt(-1)).to eq(nil)
      expect(heap.pop_lt(5)).to eq({i: 0})
      expect(heap.pop_lt(5)).to eq({i: 1})
      expect(heap.pop_lt(5)).to eq({i: 2})
      expect(heap.pop_lt(5)).to eq({i: 3})
      expect(heap.pop_lt(5)).to eq({i: 4})
      expect(heap.pop_lt(5)).to eq(nil)
    end
  end

end
