# frozen_string_literal: true

RSpec.describe DHeap do
  describe_any_size_heap "#pop_all_below(max_score, array = [])" do

    let(:n) { 10_000 }

    let(:values) {
      # just creating generic sentinal values
      Array.new(n) { Object.new }
    }

    let(:scores) {
      Array.new(n) {|i| Time.now + i }
    }

    let(:values_and_scores) { values.zip(scores).shuffle }

    before(:each) do
      values_and_scores.each do |value, score|
        heap.push value, score
      end
    end

    it "returns empty array when max_score is below all scores" do
      expect(heap.pop_all_below(scores[0] - 1)).to eq([])
      expect(heap.size).to eq(values.size)
    end

    it "pops an array with all items having score <= max_score" do
      expect(heap.pop_all_below(scores[5])).to eq(values[0, 5])
      expect(heap.size).to eq(values.size - 5)
      expect(heap.peek).to eq(values[5])
    end

    it "pushes values into an array that is provided" do
      array0 = %i[pre existing values]
      heap.pop_all_below(scores[2], array0)
      expect(array0).to eq(
        [:pre, :existing, :values, values[0], values[1]]
      )
      array1 = heap.pop_all_below(scores[4], array0)
      expect(array0).to equal(array1)
      expect(array1).to eq(
        [:pre, :existing, :values, values[0], values[1], values[2], values[3]]
      )
    end

    it "pushes values into any object that responds to <<" do
      array = instance_double(Array)
      allow(array).to receive(:<<)
      heap.pop_all_below(scores[3], array)
      expect(array).to have_received(:<<).with(values[0]).ordered
      expect(array).to have_received(:<<).with(values[1]).ordered
      expect(array).to have_received(:<<).with(values[2]).ordered
      heap.pop_all_below(scores[5], array)
      expect(array).to have_received(:<<).with(values[3]).ordered
      expect(array).to have_received(:<<).with(values[4]).ordered
    end

    it "raises NoMethodError if object doesn't respond to <<" do
      object = Object.new
      expect{ heap.pop_all_below(scores[3], object) }
        .to raise_error(NoMethodError, /<</)
    end

  end
end
