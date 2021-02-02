# frozen_string_literal: true

return unless defined?(DHeap::Map)

RSpec.describe DHeap::Map do

  it "is a subclass of DHeap" do
    expect(DHeap::Map).to be < DHeap
  end

  subject(:heap) { DHeap::Map.new }

  it "correctly heap sorts many random unique values" do
    200.times do
      # scores = Array.new(10_000) { rand(0..10_000_000) }.uniq  # =>
      scores = Array.new(20) { rand(0..10_000) }.uniq
      values = Array.new(scores.size, &:to_s)
      unsorted = values.zip(scores)
      unsorted.each do |v, s|
        heap.push(v, s)
      end
      popped = []
      while (valscore = heap.pop_with_score)
        popped << valscore
      end
      expect(popped).to eq(unsorted.sort_by(&:last))
    end
  end

  it "uniquely heap sorts many random repeated values" do
    200.times do
      # scores = Array.new(10_000) { rand(0..10_000_000) }.uniq
      scores = Array.new(20) { rand(0..10_000) }.uniq
      values = Array.new(scores.size) { rand(1_000_000) }
      unsorted = values.zip(scores)
      hash = {}
      unsorted.each do |v, s|
        hash[v] = s
        heap.push(v, s)
        # p heap.to_a; # check!(heap)
      end
      popped = []
      while (valscore = heap.pop_with_score)
        popped << valscore
      end
      sorted = hash.to_a.sort_by(&:last)
      expect(popped.size).to eq(sorted.size)
      expect(popped).to eq(sorted)
    end
  end

  it "removes the scores when popped" do
    heap.push("foo", 1 << 34)
    heap.push("bar", 1 << 23)
    expect(heap["foo"]).to eq(1 << 34)
    expect(heap["bar"]).to eq(1 << 23)
    expect(heap.pop).to eq("bar")
    expect(heap["bar"]).to be_nil
    expect(heap.pop).to eq("foo")
    expect(heap["foo"]).to be_nil
  end

  describe "#[obj]" do

    it "can lookup scores by value" do
      heap.push("abc", 123)
      expect(heap["abc"]).to eq(123)
    end

    it "returns nil for missing values" do
      heap.push("abc", 123)
      expect(heap["cde"]).to be_nil
    end

  end

  describe "#[obj] = score" do

    it "can adjust scores down" do
      heap.push("abc", 123)
      heap.push("efg", 456)
      heap.push("hij", 789)
      heap["abc"] = 9
      heap["efg"] = 5
      heap["hij"] = 2
      expect(heap.pop_with_score).to eq(["hij", 2])
      expect(heap.pop_with_score).to eq(["efg", 5])
      expect(heap.pop_with_score).to eq(["abc", 9])
    end

    it "can adjust scores up" do
      heap.push("abc", 9)
      heap.push("efg", 5)
      heap.push("hij", 2)
      heap["abc"] = 123
      heap["efg"] = 456
      heap["hij"] = 789
      expect(heap.pop_with_score).to eq(["abc", 123])
      expect(heap.pop_with_score).to eq(["efg", 456])
      expect(heap.pop_with_score).to eq(["hij", 789])
    end

    it "can add new values" do
      heap["abc"] = 123
      heap["efg"] = 456
      heap["hij"] = 789
      expect(heap.pop_with_score).to eq(["abc", 123])
      expect(heap.pop_with_score).to eq(["efg", 456])
      expect(heap.pop_with_score).to eq(["hij", 789])
    end

  end

  describe "#clear" do
    it "removes the items from the index" do
      heap.push(Object.new, 123)
      heap.push(Object.new, 456)
      heap.push(Object.new, 789)
      heap.push("a", 987)
      i = 0
      i += 1 while heap.pop
      expect(heap["a"]).to be_nil
    end
  end

  describe "#delete(obj)" do
    it "can delete existing values"
    it "returns the score if the value was in the heap"
    it "returns nil if the value wasn't in the heap"
  end

end
