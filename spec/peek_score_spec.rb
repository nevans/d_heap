# frozen_string_literal: true

require "ostruct"

RSpec.describe DHeap do

  describe "#peek_score" do
    subject(:heap) { DHeap.new }

    it "returns the next score (but not the next item)" do
      t1 = OpenStruct.new(id: 1, to_f: (Time.now + 5 * 60).to_f)
      t2 = OpenStruct.new(id: 2, to_f: (Time.now + 50).to_f)
      t3 = OpenStruct.new(id: 3, to_f: (Time.now + 60).to_f)
      t4 = OpenStruct.new(id: 4, to_f: (Time.now + 5).to_f)

      heap << t1 << t2 << t3 << t4
      expect(heap.peek_score).to eq(t4.to_f)
      expect(heap.pop).to eq(t4)
      expect(heap.peek_score).to eq(t2.to_f)
      expect(heap.pop).to eq(t2)

      heap << -1.2 << -10_000.23
      expect(heap.peek_score).to eq(-10_000.23)
      expect(heap.pop).to eq(-10_000.23)
      expect(heap.peek_score).to eq(-1.2)
      expect(heap.pop).to eq(-1.2)
    end
  end

end
