# frozen_string_literal: true

RSpec.describe DHeap, "comparisons" do

  describe "with optimized types" do

    it "sorts 'small' integers (FIXNUM_P)" do
      heap = DHeap.new
      heap << 2**4
      heap << 2**62 - 1 # max FIXNUM
      heap << 2**16
      heap << 2**8
      heap << 2**2
      heap << 2**1
      heap << 2**32
      expect(heap.pop).to eq 2**1
      expect(heap.pop).to eq 2**2
      expect(heap.pop).to eq 2**4
      expect(heap.pop).to eq 2**8
      expect(heap.pop).to eq 2**16
      expect(heap.pop).to eq 2**32
      expect(heap.pop).to eq 2**62 - 1
      expect(heap).to be_empty
    end

    it "sorts 'big' integers (FIXNUM_P)" do
      heap = DHeap.new
      heap << 2**66
      heap << 2**63
      heap << 2**67
      heap << 2**65
      heap << 2**64
      heap << 2**62
      expect(heap.pop).to eq 2**62
      expect(heap.pop).to eq 2**63
      expect(heap.pop).to eq 2**64
      expect(heap.pop).to eq 2**65
      expect(heap.pop).to eq 2**66
      expect(heap.pop).to eq 2**67
      expect(heap).to be_empty
    end

    # It looks like rb_float_cmp works? Is this safe on all platforms?
    it "sorts floats" do
      heap = DHeap.new
      20.times do heap << rand(10.0...100.0) end
      array = []
      array << heap.pop until heap.empty?
      expect(array).to eq(array.sort)
      expect(array.length).to eq(20)
    end

    it "sorts strings (bitwise then by encoding)" do
      xff1 = [0xFF].pack("C").force_encoding("utf-8")
      xff2 = [0xFF].pack("C").force_encoding("iso-8859-1")
      heap = DHeap.new
      heap << "abc"
      heap << "ÄÖÛ"
      heap << xff1
      heap << "123"
      heap << "ÄÖÜ"
      heap << "do re mi"
      heap << xff2
      heap << "ABC"
      expect(heap.pop).to eq "123"
      expect(heap.pop).to eq "ABC"
      expect(heap.pop).to eq "abc"
      expect(heap.pop).to eq "do re mi"
      expect(heap.pop).to eq "ÄÖÛ"
      expect(heap.pop).to eq "ÄÖÜ"
      expect(heap.pop).to eq xff1
      expect(heap.pop).to eq xff2
      expect(heap).to be_empty
    end

  end

  it "sorts heterogeneous comparable numbers" do
    heap = DHeap.new
    10.times do heap << Rational(rand(1..100), rand(1..1000)) end
    10.times do heap << rand(1..1000) end
    10.times do heap << rand(10.0...100.0) end
    array = []
    array << heap.pop until heap.empty?
    expect(array).to eq(array.sort)
    expect(array.length).to eq(30)
  end

  # n.b. I tried to use rb_rational_cmp, but that symbol isn't exported
  it "sorts rationals" do
    heap = DHeap.new
    20.times do heap << Rational(rand(1..100), rand(1..1000)) end
    array = []
    array << heap.pop until heap.empty?
    expect(array).to eq(array.sort)
    expect(array.length).to eq(20)
  end

end
