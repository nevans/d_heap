# frozen_string_literal: true

RSpec.describe DHeap do
  it "has a version number" do
    expect(DHeap::VERSION).not_to be nil
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

  describe_any_size_heap "pushing" do

    it "retains min peek for one element" do
      t = Time.now
      heap.push(t)
      expect(heap).to_not be_empty
      expect(heap.peek).to eq(t)
      expect(heap.size).to eq(1)
    end

    it "retains min peek for two elements in-order" do
      t0 = Time.now
      t1 = Time.now + 0.001
      heap.push(t0)
      heap.push(t1)
      expect(heap.peek).to eq(t0)
      expect(heap.size).to eq(2)
    end

    it "retains min peek for two elements reversed" do
      t0 = Time.now
      t1 = Time.now + 0.001
      heap.push(t1)
      heap.push(t0)
      expect(heap.peek).to eq(t0)
      expect(heap.size).to eq(2)
    end

    it "retains min peek for three elements in-order" do
      t0 = Time.now
      t1 = Time.now + 0.001
      t2 = Time.now + 0.002
      heap.push(t0)
      heap.push(t1)
      heap.push(t2)
      expect(heap.peek).to eq(t0)
      expect(heap.size).to eq(3)
    end

    it "retains min peek for three elements reversed" do
      t0 = Time.now
      t1 = Time.now + 0.001
      t2 = Time.now + 0.002
      heap.push(t2)
      heap.push(t1)
      heap.push(t0)
      expect(heap.peek).to eq(t0)
      expect(heap.size).to eq(3)
    end

    it "retains min peek for many elements in random order" do
      1000.times do heap.push(rand(1..9999)) end
      heap.push(-1)
      1000.times do heap.push(rand(1..9999)) end
      expect(heap.size).to eq(2001)
      expect(heap.peek).to eq(-1)
    end

    it "can insert a value with a score" do
      heap.insert(0.0, obj0 = Object.new)
      heap.insert(0.1, Object.new)
      heap.insert(0.2, Object.new)
      expect(heap.peek).to eq(obj0)
    end

    it "can push a score with a related value" do
      heap.push(obj0 = Object.new, 0.0)
      heap.push(Object.new, 0.1)
      heap.push(Object.new, 0.2)
      expect(heap.peek).to eq(obj0)
    end

    it "can enq a score with a related value" do
      heap.enq(obj0 = Object.new, 0.0)
      heap.enq(Object.new, 0.1)
      heap.enq(Object.new, 0.2)
      expect(heap.peek).to eq(obj0)
    end

    it "can push any value that returns a float from #to_f" do
      klass = Struct.new(:to_f)
      heap << 1
      heap << (obj = klass.new(-99.999))
      heap << (t = Time.now)
      heap << "1.2"
      expect { heap << klass.new("not a float") }.to raise_error(TypeError)
      expect { heap << "not a float" }.to raise_error(ArgumentError)
      expect(heap.pop).to equal(obj)
      expect(heap.pop).to equal(1)
      expect(heap.pop).to equal("1.2")
      expect(heap.pop).to equal(t)
    end

  end

  describe_any_size_heap "#pop" do
    context "with elements inserted in order" do
      include_examples "it pops all elements in order", 10, :sort
    end

    context "with elements inserted in reverse" do
      include_examples "it pops all elements in order", 100, :reverse
    end

    context "with many elements inserted randomly" do
      include_examples "it pops all elements in order", 9999, :shuffle
    end
  end

end
