# frozen_string_literal: true

module DHeap::Benchmarks

  # base class for example priority queues
  class ExamplePriorityQueue
    attr_reader :a

    def initialize
      @a = []
    end

    def clear
      @a.clear
    end

    def empty?
      @a.empty?
    end

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
  # rubocop:disable Metrics/MethodLength, Metrics/AbcSize
  class RbHeap < ExamplePriorityQueue

    def <<(score)
      raise ArgumentError unless score
      @a.push(score)
      # shift up
      index = @a.size - 1
      while 0 < index # rubocop:disable Style/NumericPredicate
        parent_index = (index - 1) / 2
        break if @a[parent_index] <= @a[index]
        @a[index] = @a[parent_index]
        index = parent_index
        @a[index] = score
        # check_heap!(index)
      end
      self
    end

    def pop
      return if @a.empty?
      popped = @a.first
      @a[0] = shifting = @a.last
      @a.pop
      # shift down
      index = 0
      last_index = @a.size - 1
      while (child_index = index * 2 + 1) <= last_index
        # select min child
        if child_index < last_index && @a[child_index + 1] < @a[child_index]
          child_index += 1
        end
        break if @a[index] <= @a[child_index]
        @a[index] = @a[child_index]
        index = child_index
        @a[index] = shifting
      end
      popped
    end

    private

    def check_heap!(idx, last = @a.size - 1)
      pscore = @a[idx]
      child = idx * 2 + 1
      if child <= last
        cscore = check_heap!(child)
        raise "#{pscore} > #{cscore}" if pscore > cscore
      end
      child += 1
      if child <= last
        check_heap!(child)
        cscore = check_heap!(child)
        raise "#{pscore} > #{cscore}" if pscore > cscore
      end
      pscore
    end

  end
  # rubocop:enable Metrics/MethodLength, Metrics/AbcSize

  # Different duck-typed priority queue implemenations
  IMPLEMENTATIONS = [
    OpenStruct.new(name: " push and resort", klass: Sorting).freeze,
    OpenStruct.new(name: "  find min + del", klass: FindMin).freeze,
    OpenStruct.new(name: "bsearch + insert", klass: BSearch).freeze,
    OpenStruct.new(name: "ruby binary heap", klass: RbHeap).freeze,
    OpenStruct.new(name: "quaternary DHeap", klass: DHeap).freeze,
  ].freeze

end
