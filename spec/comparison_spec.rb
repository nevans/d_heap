# frozen_string_literal: true

RSpec.describe DHeap, "comparisons" do

  describe "with optimized types" do

    it "sorts 'small' integers (T_FIXNUM)" do
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

    it "sorts 'big' integers (T_BIGNUM) if they fit in unsigned long long " do
      # every bit set at various bit widths
      char_min   = -(1 <<  7)
      char_max   =  (1 <<  7) - 1
      uchar_max  =  (1 <<  8) - 1

      short_min  = -(1 << 15)
      short_max  =  (1 << 15) - 1
      ushort_max =  (1 << 16) - 1

      long_min   = -(1 << 31)
      long_max   =  (1 << 31) - 1
      ulong_max  =  (1 << 32) - 1

      fixnum_min = -(2**(0.size * 8 - 2))     # on a 64bit system: -(2**62)
      fixnum_max =  (2**(0.size * 8 - 2)) - 1 # on a 64bit system:  (2**62)-1

      llong_min  = -(1 << 63) - 1
      llong_max  =  (1 << 63) - 1
      ullong_max =  (1 << 64) - 1

      # These represent the mantissa so they are the largest *consecutive* ints
      # The sign bit is stored separately, so this is the largest absolute val.
      flo_mantissa = (1 << 24) - 1
      dbl_mantissa = (1 << 53) - 1

      heap = DHeap.new
      heap << char_max
      heap << char_min
      heap << long_max
      heap << long_min
      heap << short_max
      heap << short_min
      heap << uchar_max
      heap << ushort_max
      heap << ulong_max
      heap << -uchar_max
      heap << -ushort_max
      heap << -ulong_max
      # floating point mantissas
      heap <<  flo_mantissa
      heap << -flo_mantissa
      heap <<  dbl_mantissa
      heap << -dbl_mantissa
      heap << 0.0.next_float
      heap << 0
      heap << 0.0.prev_float

      # automatically convert Integer outside dbl_mantissa: will lose precision
      heap << -ullong_max
      heap << llong_max + 1
      heap << llong_max
      heap << llong_min
      heap << fixnum_min
      heap << fixnum_max
      heap << ullong_max + 1
      heap << -ullong_max - 1
      heap << llong_min - 1
      heap << ullong_max
      heap <<  ullong_max + 2.0
      heap << -ullong_max - 2.0
      heap <<  (2**129).to_f
      heap << -(2**129).to_f

      expect(heap.pop).to eq(-(2**129).to_f)
      ambiguous = Array.new(3) { heap.pop }
      expect(ambiguous).to contain_exactly(
        -ullong_max - 2.0,
        -ullong_max - 1,
        -ullong_max,
      )
      ambiguous = Array.new(2) { heap.pop }
      expect(ambiguous).to contain_exactly(
        llong_min - 1,
        llong_min,
      )
      expect(heap.pop).to eq(fixnum_min)
      expect(heap.pop).to eq(-dbl_mantissa)
      expect(heap.pop).to eq(-ulong_max)
      expect(heap.pop).to eq(long_min)
      expect(heap.pop).to eq(-flo_mantissa)
      expect(heap.pop).to eq(-ushort_max)
      expect(heap.pop).to eq(short_min)
      expect(heap.pop).to eq(-uchar_max)
      expect(heap.pop).to eq(char_min)
      expect(heap.pop).to eq(0.0.prev_float)
      expect(heap.pop).to eq(0)
      expect(heap.pop).to eq(0.0.next_float)
      expect(heap.pop).to eq(char_max)
      expect(heap.pop).to eq(uchar_max)
      expect(heap.pop).to eq(short_max)
      expect(heap.pop).to eq(ushort_max)
      expect(heap.pop).to eq(flo_mantissa)
      expect(heap.pop).to eq(long_max)
      expect(heap.pop).to eq(ulong_max)
      expect(heap.pop).to eq(dbl_mantissa)
      expect(heap.pop).to eq(fixnum_max)
      ambiguous = Array.new(2) { heap.pop }
      expect(ambiguous).to contain_exactly(
        llong_max,
        llong_max + 1,
      )
      ambiguous = Array.new(3) { heap.pop }
      expect(ambiguous).to contain_exactly(
        ullong_max,
        ullong_max + 1,
        ullong_max + 2.0,
      )
      expect(heap.pop).to eq((2**129).to_f)
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

    # Only allowing floats, for now... but might bring this back later
    # it "sorts strings (bitwise then by encoding)" do
    #   xff1 = [0xFF].pack("C").force_encoding("utf-8")
    #   xff2 = [0xFF].pack("C").force_encoding("iso-8859-1")
    #   heap = DHeap.new
    #   heap << "abc"
    #   heap << "ÄÖÛ"
    #   heap << xff1
    #   heap << "123"
    #   heap << "ÄÖÜ"
    #   heap << "do re mi"
    #   heap << xff2
    #   heap << "ABC"
    #   expect(heap.pop).to eq "123"
    #   expect(heap.pop).to eq "ABC"
    #   expect(heap.pop).to eq "abc"
    #   expect(heap.pop).to eq "do re mi"
    #   expect(heap.pop).to eq "ÄÖÛ"
    #   expect(heap.pop).to eq "ÄÖÜ"
    #   expect(heap.pop).to eq xff1
    #   expect(heap.pop).to eq xff2
    #   expect(heap).to be_empty
    # end

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
