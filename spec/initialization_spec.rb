# frozen_string_literal: true

RSpec.describe DHeap do

  it "defaults to d=#{DHeap::DEFAULT_D}" do
    expect(DHeap.new.d).to eq(DHeap::DEFAULT_D)
  end

  describe_any_size_heap "initialization" do |dval|

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
end
