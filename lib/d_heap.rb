# frozen_string_literal: true

require "d_heap/d_heap"
require "d_heap/version"

# A fast _d_-ary heap implementation for ruby, useful in priority queues and graph
# algorithms.
#
# The _d_-ary heap data structure is a generalization of the binary heap, in which
# the nodes have _d_ children instead of 2.  This allows for "decrease priority"
# operations to be performed more quickly with the tradeoff of slower delete
# minimum.  Additionally, _d_-ary heaps can have better memory cache behavior than
# binary heaps, allowing them to pop more quickly in practice despite slower
# worst-case time complexity.
#
# Although _d_ can be configured when creating the heap, it's usually best to
# keep the default value of 4, because d=4 gives the smallest coefficient for
# <tt>(d + 1) log n / log d</tt> result.  As always, use benchmarks for your
# particular use-case.
#
# @example Basic push, peek, and pop
#     # create some example objects to place in our heap
#     Task = Struct.new(:id, :time) do
#       def to_f; time.to_f end
#     end
#     t1 = Task.new(1, Time.now + 5*60)
#     t2 = Task.new(2, Time.now + 50)
#     t3 = Task.new(3, Time.now + 60)
#     t4 = Task.new(4, Time.now +  5)
#
#     # create the heap
#     require "d_heap"
#     heap = DHeap.new
#
#     # push with an explicit score (which might be extrinsic to the value)
#     heap.push t1, t1.to_f
#
#     # the score will be implicitly cast with Float, so any object with #to_f
#     heap.push t2, t2
#
#     # if the object has an intrinsic score via #to_f, "<<" is the simplest API
#     heap << t3 << t4
#
#     # pop returns the lowest scored item, and removes it from the heap
#     heap.pop    # => #<struct Task id=4, time=2021-01-17 17:02:22.5574 -0500>
#     heap.pop    # => #<struct Task id=2, time=2021-01-17 17:03:07.5574 -0500>
#
#     # peek returns the lowest scored item, without removing it from the heap
#     heap.peek   # => #<struct Task id=3, time=2021-01-17 17:03:17.5574 -0500>
#     heap.pop    # => #<struct Task id=3, time=2021-01-17 17:03:17.5574 -0500>
#
#     # pop_lte handles the common "h.pop if h.peek_score < max" pattern
#     heap.pop_lte(Time.now + 65) # => nil
#
#     # the heap size can be inspected with size and empty?
#     heap.empty? # => false
#     heap.size   # => 1
#     heap.pop    # => #<struct Task id=1, time=2021-01-17 17:07:17.5574 -0500>
#     heap.empty? # => true
#     heap.size   # => 0
#
#     # popping from an empty heap returns nil
#     heap.pop    # => nil
#
class DHeap
  alias deq        pop
  alias shift      pop
  alias next       pop
  alias pop_all_lt pop_all_below
  alias pop_below  pop_lt

  alias enq        push

  alias first      peek

  alias length     size
  alias count      size

  # Initialize a _d_-ary min-heap.
  #
  # @param d [Integer] Number of children for each parent node.
  #          Higher values generally speed up push but slow down pop.
  #          If all pushes are popped, the default is probably best.
  # @param capacity [Integer] initial capacity of the heap.
  def initialize(d: DEFAULT_D, capacity: DEFAULT_CAPA) # rubocop:disable Naming/MethodParameterName
    __init_without_kw__(d, capacity)
  end

  # Consumes the heap by popping each minumum value until it is empty.
  #
  # If you want to iterate over the heap without consuming it, you will need to
  # first call +#dup+
  #
  # @param with_score [Boolean] if scores shoul also be yielded
  #
  # @yieldparam value [Object] each value that would be popped
  # @yieldparam score [Numeric] each value's score, if +with_scores+ is true
  #
  # @return [Enumerator] if no block is given
  # @return [nil] if a block is given
  def each_pop(with_scores: false)
    return to_enum(__method__, with_scores: with_scores) unless block_given?
    if with_scores
      yield(*pop_with_score) until empty?
    else
      yield pop until empty?
    end
    nil
  end

end
