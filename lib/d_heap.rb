require "d_heap/d_heap"
require "d_heap/version"

class DHeap

  def initialize_dup(other)
    super
    _ary_.replace(_ary_.dup)
  end

  def freeze
    _ary_.freeze
    super
  end

  def peek
    _ary_[0]
  end

  def empty?
    _ary_.empty?
  end

  def size
    _ary_.size
  end

  def each_in_order
    return to_enum(__method__) unless block_given?
    heap = dup
    yield val until heap.emptu?
  end

  def to_a
    _ary_.dup
  end

end
