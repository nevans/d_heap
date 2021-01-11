# frozen_string_literal: true

RSpec.describe DHeap do

  describe "#freeze" do
    it "returns itself frozen" do
      heap = DHeap.new
      expect(heap.freeze).to equal(heap)
      expect(heap).to be_frozen
    end
  end

  describe "frozen" do
    subject(:heap) { (DHeap.new << 1 << 2 << 3).freeze }

    it "can't push"   do expect { heap.push 4 }.to raise_error(FrozenError) end
    it "can't <<"     do expect { heap << 4 }.to raise_error(FrozenError) end
    it "can't clear"  do expect { heap.clear }.to raise_error(FrozenError) end
    it "can't pop"    do expect { heap.pop }.to raise_error(FrozenError) end
    it "can't pop_lt" do expect { heap.pop_lt 5 }.to raise_error(FrozenError) end
    it "can't pop_lte" do expect { heap.pop_lte 5 }.to raise_error(FrozenError) end

    it "can peek" do expect(heap.peek).to eq(1) end
    it "can return d" do expect(heap.d).to eq(4) end
    it "can return size" do expect(heap.size).to eq(3) end
    it "can return empty?" do expect(heap).not_to be_empty end

  end

end
