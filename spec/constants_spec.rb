# frozen_string_literal: true

RSpec.describe DHeap, "d" do

  describe "DHeap::DEFAULT_D" do
    it { expect(DHeap::DEFAULT_D).to eq(12) }

    it "is used for new DHeaps" do
      expect(DHeap.new.d).to eq(DHeap::DEFAULT_D)
    end
  end

  describe "DHeap::MAX_D" do
    # this will fail on architectures that don't have 32 bit int
    it { expect(DHeap::MAX_D).to eq(0x7fffffff) }
  end

  describe "validation" do

    it "isn't allow to use d < 2" do
      expect{ DHeap.new(d: -1) }.to raise_error(ArgumentError)
      expect{ DHeap.new(d: 0) }.to  raise_error(ArgumentError)
      expect{ DHeap.new(d: 1) }.to  raise_error(ArgumentError)
    end

    it "allows d = 2" do
      expect(DHeap.new(d: 2).d).to eq(2)
    end

    it "allows d = DHeap::MAX_D" do
      expect(DHeap.new(d: DHeap::MAX_D).d).to eq(DHeap::MAX_D)
    end

    it "doesn't allow d >= d=DHeap::MAX_D" do
      expect{ DHeap.new(d: DHeap::MAX_D + 1) }.to raise_error(RangeError)
    end

  end

end
