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
# binary heaps, allowing them to run more quickly in practice despite slower
# worst-case time complexity.
#
class DHeap
  alias deq       pop
  alias enq       push
  alias first     peek
  alias pop_below pop_lt

  alias length    size
  alias count     size

  # ruby 3.0+ (2.x can just use inherited initialize_clone)
  if Object.instance_method(:initialize_clone).arity == -1
    # @!visibility private
    def initialize_clone(other, freeze: nil)
      __init_clone__(other, freeze ? true : freeze)
    end
  end

  # Consumes the heap by popping each minumum value until it is empty.
  #
  # If you want to iterate over the heap without consuming it, you will need to
  # first call +#dup+
  #
  # @yieldparam value [Object] each value that would be popped
  #
  # @return [Enumerator] if no block is given
  # @return [nil] if a block is given
  def each_pop
    return to_enum(__method__) unless block_given?
    yield pop until empty?
    nil
  end

end
