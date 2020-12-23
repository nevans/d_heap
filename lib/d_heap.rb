require "d_heap/d_heap"
require "d_heap/version"

class DHeap

  def initialize_copy(other)
    raise NotImplementedError, "initialize_copy should deep copy array"
  end

end
