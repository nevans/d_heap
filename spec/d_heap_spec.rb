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
        expect(heap.peek).to eq(t)
        expect(heap.size).to eq(1)
      end

      it "retains min peek for two elements in-order" do
        t0 = Time.now
        t1 = Time.now
        expect(heap.push(t0)).to eq(0)
        expect(heap.push(t1)).to eq(1)
        expect(heap.peek).to eq(t0)
        expect(heap.size).to eq(2)
      end

      it "retains min peek for two elements reversed" do
        t0 = Time.now
        t1 = Time.now
        expect(heap.push(t1)).to eq(0)
        expect(heap.push(t0)).to eq(0)
        expect(heap.peek).to eq(t0)
        expect(heap.size).to eq(2)
      end

      it "retains min peek for three elements in-order" do
        t0 = Time.now
        t1 = Time.now
        t2 = Time.now
        expect(heap.push(t0)).to eq(0)
        expect(heap.push(t1)).to eq(1)
        expect(heap.push(t2)).to eq(2)
        expect(heap.peek).to eq(t0)
        expect(heap.size).to eq(3)
      end

      it "retains min peek for three elements reversed" do
        t0 = Time.now
        t1 = Time.now
        t2 = Time.now
        expect(heap.push(t2)).to eq(0)
        expect(heap.push(t1)).to eq(0)
        expect(heap.push(t0)).to eq(0)
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

    end

    context "popping" do
      context "with elements inserted in order" do
        include_examples "it pops all elements in order", 100, :sort
      end

      context "with elements inserted in reverse" do
        include_examples "it pops all elements in order", 100, :reverse
      end

      context "with many elements inserted randomly" do
        include_examples "it pops all elements in order", 9999, :shuffle
      end
    end

  end

  [2, 3, 4, 5, 6, 7, 8].each do |d|
    context "heap with d=#{d}" do
      include_examples "any-size heap", d
    end
  end

  describe "with an empty array" do

    describe ".heap_sift_down([], d, index)" do
      it "raises IndexError" do
        expect{ DHeap.heap_sift_down([], 4, -1) }
          .to raise_error(IndexError, /too small/)
        expect{ DHeap.heap_sift_down([], 2, 0) }
          .to raise_error(IndexError, /too large/)
      end
    end

  end

  describe ".heap_sift_down(array, d, index) => heap_index" do
    context "with d=2" do
      let(:d) { 2 }

      context "with index=0" do
        it "sifts down from the first value" do
          ary = [
            42,
            2, 3,
            4, 5, 6, 7,
            8, 9, 10, 11, 12, 13, 14, 15
          ]
          expect(DHeap.heap_sift_down(ary, d, 0)).to eq(7)
          expect(ary).to eq([
            2,
            4, 3,
            8, 5, 6, 7,
            42, 9, 10, 11, 12, 13, 14, 15
          ])
        end
      end
    end

    context "with d=4" do
      let(:d) { 4 }

      context "with index=0" do
        it "sifts down from the first value" do
          ary = [
            42,
            # layer 2:
            2, 3, 4, 5,
            # layer 3:
            6, 7, 8, 9,
            10, 11, 12, 13,
            14, 15, 16, 17,
            18, 19, 20, 21,
          ]
          expect(DHeap.heap_sift_down(ary, d, 0)).to eq(5)
          expect(ary).to eq([
            2,
            # layer 2:
            6, 3, 4, 5,
            # layer 3:
            42, 7, 8, 9,
            10, 11, 12, 13,
            14, 15, 16, 17,
            18, 19, 20, 21,
          ])
        end
      end
    end

  end

end
