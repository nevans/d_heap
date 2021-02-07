# frozen_string_literal: true

require "ostruct"

RSpec.describe DHeap do

  describe_any_size_heap "#pop_with_score" do

    it "returns an array with both item and score" do
      t1 = OpenStruct.new(id: 1, to_f: (Time.now + 5 * 60).to_f)
      t2 = OpenStruct.new(id: 2, to_f: (Time.now + 50).to_f)
      t3 = OpenStruct.new(id: 3, to_f: (Time.now + 60).to_f)
      t4 = OpenStruct.new(id: 4, to_f: (Time.now + 5).to_f)

      heap << t1 << t2 << t3 << t4
      expect(heap.pop_with_score).to eq([t4, t4.to_f])
      expect(heap.pop_with_score).to eq([t2, t2.to_f])
      expect(heap.pop_with_score).to eq([t3, t3.to_f])
      expect(heap.pop_with_score).to eq([t1, t1.to_f])

      heap << -1.2 << -10_000.23
      expect(heap.pop_with_score).to eq([-10_000.23] * 2)
      expect(heap.pop_with_score).to eq([-1.2] * 2)
    end

    let(:max_dbl_mantissa) { (1 << 53) - 1 }

    it "returns whole number scores > double mantissa (53 bits) as Float" do
      obj1 = Object.new
      obj2 = Object.new
      heap.push obj1, max_dbl_mantissa + 1
      heap.push obj2, -(max_dbl_mantissa + 1)

      value, score = heap.pop_with_score
      expect(value).to eq(obj2)
      expect(score).to eq(-(max_dbl_mantissa + 1))
      expect(score).to be_an_instance_of(Float)

      value, score = heap.pop_with_score
      expect(value).to eq(obj1)
      expect(score).to eq(max_dbl_mantissa + 1)
      expect(score).to be_an_instance_of(Float)
    end

    it "returns whole number scores <= double mantissa (53 bits) as Float" do
      obj1 = Object.new
      obj2 = Object.new
      heap.push obj1, max_dbl_mantissa.to_f
      heap.push obj2, max_dbl_mantissa - 1

      value, score = heap.pop_with_score
      expect(value).to eq(obj2)
      expect(score).to eq(max_dbl_mantissa - 1)
      expect(score).to be_an_instance_of(Float)

      value, score = heap.pop_with_score
      expect(value).to eq(obj1)
      expect(score).to eq(max_dbl_mantissa)
      expect(score).to be_an_instance_of(Float)

      value, score = heap.pop_with_score
      expect(value).to be_nil
      expect(score).to be_nil
    end

  end

end
