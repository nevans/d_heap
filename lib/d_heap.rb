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
  # ruby 3.0+ (2.x can just use inherited initialize_clone)
  if Object.instance_method(:initialize_clone).arity == -1
    def initialize_clone(other, freeze: nil)
      __init_clone__(other, freeze ? true : freeze)
    end
  end

end
