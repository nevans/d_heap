# frozen_string_literal: true

RSpec.describe DHeap do

  it "has a DEFAULT_D constant" do
    expect(DHeap::DEFAULT_D).to eq(4)
  end

  it "uses DEFAULT_D for new DHeaps" do
    expect(DHeap.new.d).to eq(DHeap::DEFAULT_D)
  end

  it "has a MAX_D constant" do
    expect(DHeap::DEFAULT_D).to eq(4)
  end

  it "doesn't allow construction with d=1 or lower" do
    expect{ DHeap.new(-1) }.to raise_error(ArgumentError)
    expect{ DHeap.new(0) }.to  raise_error(ArgumentError)
    expect{ DHeap.new(1) }.to  raise_error(ArgumentError)
  end

  it "does allow d=2 or DHeap::MAX_D" do
    expect(DHeap.new(2).d).to eq(2)
    expect(DHeap.new.d).to eq(DHeap::DEFAULT_D)
    expect(DHeap.new(DHeap::MAX_D).d).to eq(DHeap::MAX_D)
  end

  it "doesn't allow construction above d=DHeap::MAX_D" do
    expect{ DHeap.new(DHeap::MAX_D + 1) }.to raise_error(ArgumentError)
  end

end
