# frozen_string_literal: true

RSpec.describe DHeap do
  it "has a version number" do
    expect(DHeap::VERSION).not_to be nil
  end

  it "defaults to d=4" do
    expect(DHeap.new.d).to eq(4)
  end

  shared_examples "it pops all elements in order" do |max, order|
    before do
      (0..max).to_a.__send__(order).each do |i| heap << i end
    end
    it "pops all elements in order" do
      (0..max).each do |i|
        # p i: i, h: [heap.peek, *heap.to_a[1..-1].each_slice(d).to_a]
        expect(heap.pop).to eq(i)
      end
      expect(heap).to be_empty
      expect(heap.pop).to be_nil
    end
  end

  shared_examples "any-size heap" do |dval|
    let(:d) { dval }
    subject(:heap) { DHeap.new(d) }

    context "initialization" do
      it { is_expected.to be_empty }
      it "pops nil" do
        expect(heap.pop).to be_nil
      end
      it "peeks nil" do
        expect(heap.peek).to be_nil
      end
      it "has size of 0" do
        expect(heap.size).to eq(0)
      end
      it "has d=#{dval}" do
        expect(heap.d).to eq(d)
      end
    end

    context "pushing" do

      it "retains min peek for one element" do
        t = Time.now
        expect(heap.push(t)).to eq(0)
        expect(heap).to_not be_empty
        expect(heap.peek).to eq(t)
        expect(heap.size).to eq(1)
      end

      it "retains min peek for two elements in-order" do
        t0 = Time.now
        t1 = Time.now + 0.001
        expect(heap.push(t0)).to eq(0)
        expect(heap.push(t1)).to eq(1)
        expect(heap.peek).to eq(t0)
        expect(heap.size).to eq(2)
      end

      it "retains min peek for two elements reversed" do
        t0 = Time.now
        t1 = Time.now + 0.001
        expect(heap.push(t1)).to eq(0)
        expect(heap.push(t0)).to eq(0)
        expect(heap.peek).to eq(t0)
        expect(heap.size).to eq(2)
      end

      it "retains min peek for three elements in-order" do
        t0 = Time.now
        t1 = Time.now + 0.001
        t2 = Time.now + 0.002
        expect(heap.push(t0)).to eq(0)
        expect(heap.push(t1)).to eq(1)
        expect(heap.push(t2)).to eq(2)
        expect(heap.peek).to eq(t0)
        expect(heap.size).to eq(3)
      end

      it "retains min peek for three elements reversed" do
        t0 = Time.now
        t1 = Time.now + 0.001
        t2 = Time.now + 0.002
        expect(heap.push(t2)).to eq(0)
        expect(heap.push(t1)).to eq(0)
        expect(heap.push(t0)).to eq(0)
        expect(heap.peek).to eq(t0)
        expect(heap.size).to eq(3)
      end

      # rubocop:disable Layout/IndentationWidth
      unless /darwin/ =~ RUBY_PLATFORM
      it "retains min peek for many elements in random order" do
        1000.times do heap.push(rand(1..9999)) end
        heap.push(-1)
        1000.times do heap.push(rand(1..9999)) end
        expect(heap.size).to eq(2001)
        expect(heap.peek).to eq(-1)
      end
      end
      # rubocop:enable Layout/IndentationWidth

      it "can push a score with a related value" do
        expect(heap.push("score 0", obj0 = Object.new))
        expect(heap.push("score 1", Object.new))
        expect(heap.push("score 2", Object.new))
        expect(heap.peek).to eq(obj0)
      end

    end

    describe "#pop" do
      context "with elements inserted in order" do
        include_examples "it pops all elements in order", 10, :sort
      end

      context "with elements inserted in reverse" do
        include_examples "it pops all elements in order", 100, :reverse
      end

      # rubocop:disable Layout/IndentationWidth
      unless /darwin/ =~ RUBY_PLATFORM
      context "with many elements inserted randomly" do
        include_examples "it pops all elements in order", 9999, :shuffle
      end
      end
      # rubocop:enable Layout/IndentationWidth
    end

    describe "#pop_lte(n)" do
      it "pops only the elements that are compare as <= n" do
        (0..20).to_a.shuffle.each do |i| heap.push i, {i: i} end
        expect(heap.pop_lte(-1)).to eq(nil)
        expect(heap.pop_lte(4)).to eq({i: 0})
        expect(heap.pop_lte(4)).to eq({i: 1})
        expect(heap.pop_lte(4)).to eq({i: 2})
        expect(heap.pop_lte(4)).to eq({i: 3})
        expect(heap.pop_lte(4)).to eq({i: 4})
        expect(heap.pop_lte(4)).to eq(nil)
      end
    end

    describe "#pop_lt(n)" do
      it "pops only the elements that are compare as < n" do
        (0..20).to_a.shuffle.each do |i| heap.push i, {i: i} end
        expect(heap.pop_lt(-1)).to eq(nil)
        expect(heap.pop_lt(5)).to eq({i: 0})
        expect(heap.pop_lt(5)).to eq({i: 1})
        expect(heap.pop_lt(5)).to eq({i: 2})
        expect(heap.pop_lt(5)).to eq({i: 3})
        expect(heap.pop_lt(5)).to eq({i: 4})
        expect(heap.pop_lt(5)).to eq(nil)
      end
    end

    describe "#clear" do
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

  [2, 3, 4, 5, 6].each do |d|
    context "heap with d=#{d}" do
      include_examples "any-size heap", d
    end
  end

end
