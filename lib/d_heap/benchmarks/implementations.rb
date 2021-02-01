# frozen_string_literal: true

require "fc"

module DHeap::Benchmarks

  # base class for example priority queues
  class ExamplePriorityQueue
    attr_reader :a

    # quick initialization by simply sorting the array once.
    def initialize(count = nil, &block)
      @a = []
      return unless count
      count.times {|i| @a << block.call(i) }
      @a.sort!
    end

    def clear; @a.clear end
    def empty?; @a.empty? end
    def size; @a.size end

    if ENV["LOG_LEVEL"] == "debug"
      def dbg(msg)
        puts "%20s: %p, %p" % [msg, @a.first, (@a[1..-1] || []).each_slice(2).to_a]
      end
    else
      def dbg(msg) nil end
    end

  end

  # The most naive approach--completely unsorted!--is ironically not the worst.
  class FindMin < ExamplePriorityQueue

    # O(1)
    def <<(score)
      raise ArgumentError unless score
      @a.push score
    end

    # O(n)
    def pop
      return unless (score = @a.min)
      index = @a.rindex(score)
      @a.delete_at(index)
      score
    end

  end

  # Re-sorting after each insert: this both naive and performs the worst.
  class Sorting < ExamplePriorityQueue

    # O(n log n)
    def <<(score)
      raise ArgumentError unless score
      @a.push score
      @a.sort!
    end

    # O(1)
    def pop
      @a.shift
    end

  end

  # A very simple example priority queue that is implemented with a sorted array.
  #
  # It uses Array#bsearch + Array#insert to push new values, and Array#pop to pop
  # the min value.
  class BSearch < ExamplePriorityQueue

    # Array#bsearch_index is O(log n)
    # Array#insert        is O(n)
    #
    # So this should be O(n).
    #
    # In practice though, memcpy has a *very* small constant factor.
    # And bsearch_index uses *exactly* (log n / log 2) comparisons.
    def <<(score)
      raise ArgumentError unless score
      index = @a.bsearch_index {|other| score > other } || @a.length
      @a.insert(index, score)
    end

    # Array#pop is O(1). It updates length without changing capacity or contents.
    #
    # No comparisons are necessary.
    #
    # shift is usually also O(1) and could be used if it were sorted normally.
    def pop
      @a.pop
    end

  end

  # a very simple pure ruby binary heap
  class RbHeap < ExamplePriorityQueue

    # huge speedup from inlining the sift methods.
    # (no speedup from partial unrolling of the loops.)
    #
    # rubocop:disable Metrics/MethodLength, Metrics/AbcSize

    def <<(value)
      raise ArgumentError unless value
      index = @a.size
      while 0 < index # rubocop:disable Style/NumericPredicate
        parent_index = (index - 1) / 2
        parent_value = @a[parent_index]
        break if parent_value <= value
        @a[index] = parent_value
        index = parent_index
      end
      @a[index] = value
      self
    end

    def pop
      return if @a.empty?
      popped = @a.first
      value = @a.pop
      last_index = @a.size - 1
      return popped unless 0 <= last_index

      last_parent = (last_index - 1) / 2
      index = 0

      child_index = 1
      while index <= last_parent
        child_value = @a[child_index]
        if child_index < last_index && @a[child_index + 1] < child_value
          child_index += 1
          child_value = @a[child_index]
        end
        break if value <= child_value
        @a[index] = child_value
        index = child_index
        child_index = index * 2 + 1
      end
      @a[index] = value
      popped
    end

    # rubocop:enable Metrics/MethodLength, Metrics/AbcSize

    private

    def check_heap!(idx)
      check_heap_up!(idx)
      check_heap_dn!(idx)
    end

    # compares index to its parent
    def check_heap_at!(idx)
      value = @a[idx]
      unless idx <= 0
        pidx = (idx - 1) / 2
        pval = @a[pidx]
        raise "@a[#{idx}] == #{value}, #{pval} > #{value}" if pval > value
      end
      value
    end

    def check_heap_up!(idx)
      return if idx <= 0
      pidx = (idx - 1) / 2
      check_heap_at!(pidx)
      check_heap_up!(pidx)
    end

    def check_heap_dn!(idx)
      return unless @a.size <= idx
      check_heap_at!(idx)
      check_heap_down!(idx * 2 + 1)
      check_heap_down!(idx * 2 + 2)
    end

  end

  # minor adjustments to the "priority_queue_cxx" gem, to match the API
  class CppSTL

    def initialize
      clear
    end

    def <<(value); @q.push(value, value) end

    def clear
      @q = FastContainers::PriorityQueue.new(:min)
    end

    def empty?; @q.empty? end
    def size;   @q.size   end

    def pop
      @q.pop
    rescue RuntimeError
      nil
    end

  end

  # Different duck-typed priority queue implemenations
  IMPLEMENTATIONS = [
    OpenStruct.new(name: " push and resort", klass: Sorting).freeze,
    OpenStruct.new(name: "  find min + del", klass: FindMin).freeze,
    OpenStruct.new(name: "bsearch + insert", klass: BSearch).freeze,
    OpenStruct.new(name: "ruby binary heap", klass: RbHeap).freeze,
    OpenStruct.new(name: "C++STL PriorityQ", klass: CppSTL).freeze,
    OpenStruct.new(name: "quaternary DHeap", klass: DHeap).freeze,
  ].freeze

end
