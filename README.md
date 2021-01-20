# DHeap - Fast d-ary heap for ruby

[![Gem Version](https://badge.fury.io/rb/d_heap.svg)](https://badge.fury.io/rb/d_heap)
[![Build Status](https://github.com/nevans/d_heap/workflows/CI/badge.svg)](https://github.com/nevans/d_heap/actions?query=workflow%3ACI)
[![Maintainability](https://api.codeclimate.com/v1/badges/ff274acd0683c99c03e1/maintainability)](https://codeclimate.com/github/nevans/d_heap/maintainability)

A fast [_d_-ary heap][d-ary heap] [priority queue] implementation for ruby,
implemented as a C extension.

From [wikipedia](https://en.wikipedia.org/wiki/Heap_(data_structure)):
> A heap is a specialized tree-based data structure which is essentially an
> almost complete tree that satisfies the heap property: in a min heap, for any
> given node C, if P is a parent node of C, then the key (the value) of P is
> less than or equal to the key of C. The node at the "top" of the heap (with no
> parents) is called the root node.

![tree representation of a min heap](images/wikipedia-min-heap.png)

With a regular queue, you expect "FIFO" behavior: first in, first out.  With a
stack you expect "LIFO": last in first out.  A priority queue has a score for
each element and elements are popped in order by score.  Priority queues are
often used in algorithms for e.g. [scheduling] of timers or bandwidth
management, for [Huffman coding], and various graph search algorithms such as
[Dijkstra's algorithm], [A* search], or [Prim's algorithm].

The _d_-ary heap data structure is a generalization of the [binary heap], in
which the nodes have _d_ children instead of 2.  This allows for "insert" and
"decrease priority" operations to be performed more quickly with the tradeoff of
slower delete minimum or "increase priority".  Additionally, _d_-ary heaps can
have better memory cache behavior than binary heaps, allowing them to run more
quickly in practice despite slower worst-case time complexity. In the worst
case, a _d_-ary heap requires only `O(log n / log d)` operations to push, with
the tradeoff that pop requires `O(d log n / log d)`.

Although you should probably just use the default _d_ value  of `4` (see the
analysis below), it's always advisable to benchmark your specific use-case.  In
particular, if you push items more than you pop, higher values for _d_ can give
a faster total runtime.

[d-ary heap]: https://en.wikipedia.org/wiki/D-ary_heap
[priority queue]: https://en.wikipedia.org/wiki/Priority_queue
[binary heap]: https://en.wikipedia.org/wiki/Binary_heap
[scheduling]: https://en.wikipedia.org/wiki/Scheduling_(computing)
[Huffman coding]: https://en.wikipedia.org/wiki/Huffman_coding#Compression
[Dijkstra's algorithm]: https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm#Using_a_priority_queue
[A* search]: https://en.wikipedia.org/wiki/A*_search_algorithm#Description
[Prim's algorithm]: https://en.wikipedia.org/wiki/Prim%27s_algorithm

## Usage

The basic API is `#push(object, score)` and `#pop`.  Please read the
[gem documentation] for more details and other methods.

Quick reference for some common methods:

* `heap << object` adds a value, with `Float(object)` as its score.
* `heap.push(object, score)` adds a value with an extrinsic score.
* `heap.pop` removes and returns the value with the minimum score.
* `heap.pop_lte(max_score)` pops only if the next score is `<=` the argument.
* `heap.peek` to view the minimum value without popping it.
* `heap.clear` to remove all items from the heap.
* `heap.empty?` returns true if the heap is empty.
* `heap.size` returns the number of items in the heap.

If the score changes while the object is still in the heap, it will not be
re-evaluated again.

The score must either be `Integer` or `Float` or convertable to a `Float` via
`Float(score)` (i.e. it should implement `#to_f`).  Constraining scores to
numeric values gives more than 50% speedup under some benchmarks!  _n.b._
`Integer` _scores must have an absolute value that fits into_ `unsigned long
long`. This is compiler and architecture dependant but with gcc on an IA-64
system it's 64 bits, which gives a range of -18,446,744,073,709,551,615 to
+18,446,744,073,709,551,615, which is more than enough to store e.g. POSIX time
in nanoseconds.

_Comparing arbitary objects via_ `a <=> b` _was the original design and may be
added back in a future version,_ if (and only if) _it can be done without
impacting the speed of numeric comparisons. The speedup from this constraint is
huge!_

[gem documentation]: https://rubydoc.info/gems/d_heap/DHeap

### Examples

```ruby
# create some example objects to place in our heap
Task = Struct.new(:id, :time) do
  def to_f; time.to_f end
end
t1 = Task.new(1, Time.now + 5*60)
t2 = Task.new(2, Time.now + 50)
t3 = Task.new(3, Time.now + 60)
t4 = Task.new(4, Time.now +  5)

# create the heap
require "d_heap"
heap = DHeap.new

# push with an explicit score (which might be extrinsic to the value)
heap.push t1, t1.to_f

# the score will be implicitly cast with Float, so any object with #to_f
heap.push t2, t2

# if the object has an intrinsic score via #to_f, "<<" is the simplest API
heap << t3 << t4

# pop returns the lowest scored item, and removes it from the heap
heap.pop    # => #<struct Task id=4, time=2021-01-17 17:02:22.5574 -0500>
heap.pop    # => #<struct Task id=2, time=2021-01-17 17:03:07.5574 -0500>

# peek returns the lowest scored item, without removing it from the heap
heap.peek   # => #<struct Task id=3, time=2021-01-17 17:03:17.5574 -0500>
heap.pop    # => #<struct Task id=3, time=2021-01-17 17:03:17.5574 -0500>

# pop_lte handles the common "h.pop if h.peek_score < max" pattern
heap.pop_lte(Time.now + 65) # => nil

# the heap size can be inspected with size and empty?
heap.empty? # => false
heap.size   # => 1
heap.pop    # => #<struct Task id=1, time=2021-01-17 17:07:17.5574 -0500>
heap.empty? # => true
heap.size   # => 0

# popping from an empty heap returns nil
heap.pop    # => nil
```

Please see the [gem documentation] for more methods and more examples.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'd_heap'
```

And then execute:

    $ bundle install

Or install it yourself as:

    $ gem install d_heap

## Motivation

One naive approach to a priority queue is to maintain an array in sorted order.
This can be very simply implemented in ruby with `Array#bseach_index` +
`Array#insert`.  This can be very fast—`Array#pop` is `O(1)`—but the worst-case
for insert is `O(n)` because it may need to `memcpy` a significant portion of
the array.

The standard way to implement a priority queue is with a binary heap.  Although
this increases the time complexity for `pop` alone, it reduces the combined time
compexity for the combined `push` + `pop`. Using a d-ary heap with d > 2
makes the tree shorter but broader, which reduces  to `O(log n / log d)` while
increasing the comparisons needed by sift-down to `O(d log n/ log d)`.

However, I was disappointed when my best ruby heap implementation ran much more
slowly than the naive approach—even for heaps containing ten thousand items.
Although it _is_ `O(n)`, `memcpy` is _very_ fast, while calling `<=>` from ruby
has _much_ higher overhead.  And a _d_-heap needs `d + 1` times more comparisons
for each push + pop than `bsearch` + `insert`.

Additionally, when researching how other systems handle their scheduling, I was
inspired by reading go's "timer.go" implementation to experiment with a 4-ary
heap instead of the traditional binary heap.

## Benchmarks

_See `bin/benchmarks` and `docs/benchmarks.txt`, as well as `bin/profile` and
`docs/profile.txt` for much more detail or updated results. These benchmarks
were measured with v0.5.0 and ruby 2.7.2 without MJIT enabled._

These benchmarks use very simple implementations for a pure-ruby heap and an
array that is kept sorted using `Array#bsearch_index` and `Array#insert`.  For
comparison, I also compare to the [priority_queue_cxx gem] which uses the [C++
STL priority_queue], and another naive implementation that uses `Array#min` and
`Array#delete_at` with an unsorted array.

In these benchmarks, `DHeap` runs faster than all other implementations for
every scenario and every value of N, although the difference is usually more
noticable at higher values of N.  The pure ruby heap implementation is
competitive for `push` alone at every value of N, but is significantly slower
than bsearch + insert for push + pop, until N is _very_ large (somewhere between
10k and 100k)!

[priority_queue_cxx gem]: https://rubygems.org/gems/priority_queue_cxx
[C++ STL priority_queue]: http://www.cplusplus.com/reference/queue/priority_queue/

Three different scenarios are measured:

### push N items onto an empty heap

...but never pop (clearing between each set of pushes).

![bar graph for push_n_pop_n benchmarks](./images/push_n.png)

### push N items onto an empty heap then pop all N

Although this could be used for heap sort, we're unlikely to choose heap sort
over Ruby's quick sort implementation. I'm using this scenario to represent
the amortized cost of creating a heap and (eventually) draining it.

![bar graph for push_n_pop_n benchmarks](./images/push_n_pop_n.png)

### push and pop on a heap with N values

Repeatedly push and pop while keeping a stable heap size.  This is a _very
simplistic_ approximation for how most scheduler/timer heaps might be used.
Usually when a timer fires it will be quickly replaced by a new timer, and the
overall count of timers will remain roughly stable.

![bar graph for push_pop benchmarks](./images/push_pop.png)

### numbers

Even for very small N values the benchmark implementations,  `DHeap` runs faster
than the other implementations for each scenario, although the difference is
still relatively small.  The pure ruby binary heap is 2x or more slower than
bsearch + insert for common push/pop scenario.

    == push N (N=5) ==========================================================
    push N (c_dheap):   1969700.7 i/s
    push N (c++ stl):   1049738.1 i/s - 1.88x  slower
    push N (rb_heap):    928435.2 i/s - 2.12x  slower
    push N (bsearch):    921060.0 i/s - 2.14x  slower

    == push N then pop N (N=5) ===============================================
    push N + pop N (c_dheap):   1375805.0 i/s
    push N + pop N (c++ stl):   1134997.5 i/s - 1.21x  slower
    push N + pop N (findmin):    862913.1 i/s - 1.59x  slower
    push N + pop N (bsearch):    762887.1 i/s - 1.80x  slower
    push N + pop N (rb_heap):    506890.4 i/s - 2.71x  slower

    == Push/pop with pre-filled queue of size=N (N=5) ========================
    push + pop (c_dheap):   9044435.5 i/s
    push + pop (c++ stl):   7534583.4 i/s - 1.20x  slower
    push + pop (findmin):   5026155.1 i/s - 1.80x  slower
    push + pop (bsearch):   4300260.0 i/s - 2.10x  slower
    push + pop (rb_heap):   2299499.7 i/s - 3.93x  slower

By N=21, `DHeap` has pulled significantly ahead of bsearch + insert for all
scenarios, but the pure ruby heap is still slower than every other
implementation—even resorting the array after every `#push`—in any scenario that
uses `#pop`.

    == push N (N=21) =========================================================
    push N (c_dheap):    464231.4 i/s
    push N (c++ stl):    305546.7 i/s - 1.52x  slower
    push N (rb_heap):    202803.7 i/s - 2.29x  slower
    push N (bsearch):    168678.7 i/s - 2.75x  slower

    == push N then pop N (N=21) ==============================================
    push N + pop N (c_dheap):    298350.3 i/s
    push N + pop N (c++ stl):    252227.1 i/s - 1.18x  slower
    push N + pop N (findmin):    161998.7 i/s - 1.84x  slower
    push N + pop N (bsearch):    143432.3 i/s - 2.08x  slower
    push N + pop N (rb_heap):     79622.1 i/s - 3.75x  slower

    == Push/pop with pre-filled queue of size=N (N=21) =======================
    push + pop (c_dheap):   8855093.4 i/s
    push + pop (c++ stl):   7223079.5 i/s - 1.23x  slower
    push + pop (findmin):   4542913.7 i/s - 1.95x  slower
    push + pop (bsearch):   3461802.4 i/s - 2.56x  slower
    push + pop (rb_heap):   1845488.7 i/s - 4.80x  slower

At higher values of N, a heaps logarithmic growth leads to only a little
slowdown of `#push`, while insert's linear growth causes it to run noticably
slower and slower.  But because `#pop` is `O(1)` for a sorted array and `O(d log
n / log d)` for a heap, scenarios involving both `#push` and `#pop` remain
relatively close, and bsearch + insert still runs faster than a pure ruby heap,
even up to queues with 10k items.  But as queue size increases beyond than that,
the linear time compexity to keep a sorted array dominates.

    == push + pop (rb_heap)
    queue size =    10000:    736618.2 i/s
    queue size =    25000:    670186.8 i/s - 1.10x  slower
    queue size =    50000:    618156.7 i/s - 1.19x  slower
    queue size =   100000:    579250.7 i/s - 1.27x  slower
    queue size =   250000:    572795.0 i/s - 1.29x  slower
    queue size =   500000:    543648.3 i/s - 1.35x  slower
    queue size =  1000000:    513523.4 i/s - 1.43x  slower
    queue size =  2500000:    460848.9 i/s - 1.60x  slower
    queue size =  5000000:    445234.5 i/s - 1.65x  slower
    queue size = 10000000:    423119.0 i/s - 1.74x  slower

    == push + pop (bsearch)
    queue size =    10000:    786334.2 i/s
    queue size =    25000:    364963.8 i/s - 2.15x  slower
    queue size =    50000:    200520.6 i/s - 3.92x  slower
    queue size =   100000:     88607.0 i/s - 8.87x  slower
    queue size =   250000:     34530.5 i/s - 22.77x  slower
    queue size =   500000:     17965.4 i/s - 43.77x  slower
    queue size =  1000000:      5638.7 i/s - 139.45x  slower
    queue size =  2500000:      1302.0 i/s - 603.93x  slower
    queue size =  5000000:       592.0 i/s - 1328.25x  slower
    queue size = 10000000:       288.8 i/s - 2722.66x  slower

    == push + pop (c_dheap)
    queue size =    10000:   7311366.6 i/s
    queue size =    50000:   6737824.5 i/s - 1.09x  slower
    queue size =    25000:   6407340.6 i/s - 1.14x  slower
    queue size =   100000:   6254396.3 i/s - 1.17x  slower
    queue size =   250000:   5917684.5 i/s - 1.24x  slower
    queue size =   500000:   5126307.6 i/s - 1.43x  slower
    queue size =  1000000:   4403494.1 i/s - 1.66x  slower
    queue size =  2500000:   3304088.2 i/s - 2.21x  slower
    queue size =  5000000:   2664897.7 i/s - 2.74x  slower
    queue size = 10000000:   2137927.6 i/s - 3.42x  slower

## Analysis

### Time complexity

There are two fundamental heap operations: sift-up (used by push) and sift-down
(used by pop).

* A _d_-ary heap will have `log n / log d` layers, so both sift operations can
  perform as many as `log n / log d` writes, when a member sifts the entire
  length of the tree.
* Sift-up makes one comparison per layer, so push runs in `O(log n / log d)`.
* Sift-down makes d comparions per layer, so pop runs in `O(d log n / log d)`.

So, in the simplest case of running balanced push/pop while maintaining the same
heap size, `(1 + d) log n / log d` comparisons are made.  In the worst case,
when every sift traverses every layer of the tree, `d=4` requires the fewest
comparisons for combined insert and delete:

* (1 +  2) lg n / lg d ≈ 4.328085 lg n
* (1 +  3) lg n / lg d ≈ 3.640957 lg n
* (1 +  4) lg n / lg d ≈ 3.606738 lg n
* (1 +  5) lg n / lg d ≈ 3.728010 lg n
* (1 +  6) lg n / lg d ≈ 3.906774 lg n
* (1 +  7) lg n / lg d ≈ 4.111187 lg n
* (1 +  8) lg n / lg d ≈ 4.328085 lg n
* (1 +  9) lg n / lg d ≈ 4.551196 lg n
* (1 + 10) lg n / lg d ≈ 4.777239 lg n
* etc...

See https://en.wikipedia.org/wiki/D-ary_heap#Analysis for deeper analysis.

### Space complexity

Space usage is linear, regardless of d.  However higher d values may
provide better cache locality.  Because the heap is a complete binary tree, the
elements can be stored in an array, without the need for tree or list pointers.

Ruby can compare Numeric values _much_ faster than other ruby objects, even if
those objects simply delegate comparison to internal Numeric values.  And it is
often useful to use external scores for otherwise uncomparable values.  So
`DHeap` uses twice as many entries (one for score and one for value)
as an array which only stores values.

## Thread safety

`DHeap` is _not_ thread-safe, so concurrent access from multiple threads need to
take precautions such as locking access behind a mutex.

## Alternative data structures

As always, you should run benchmarks with your expected scenarios to determine
which is best for your application.

Depending on your use-case, maintaining a sorted `Array` using `#bsearch_index`
and `#insert` might be just fine!  Even `min` plus `delete` with an unsorted
array can be very fast on small queues.  Although insertions run with `O(n)`,
`memcpy` is so fast on modern hardware that your dataset might not be large
enough for it to matter.

More complex heap varients, e.g. [Fibonacci heap], allow heaps to be split and
merged which gives some graph algorithms a lower amortized time complexity.  But
in practice, _d_-ary heaps have much lower overhead and often run faster.

[Fibonacci heap]: https://en.wikipedia.org/wiki/Fibonacci_heap

If it is important to be able to quickly enumerate the set or find the ranking
of values in it, then you may want to use a self-balancing binary search tree
(e.g. a [red-black tree]) or a [skip-list].

[red-black tree]: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
[skip-list]: https://en.wikipedia.org/wiki/Skip_list

[Hashed and Heirarchical Timing Wheels][timing wheels] (or some variant in that
family of data structures) can be constructed to have effectively `O(1)` running
time in most cases.  Although the implementation for that data structure is more
complex than a heap, it may be necessary for enormous values of N.

[timing wheels]: http://www.cs.columbia.edu/~nahum/w6998/papers/ton97-timing-wheels.pdf

## TODOs...

_TODO:_ Also ~~included is~~ _will include_ `DHeap::Map`, which augments the
basic heap with an internal `Hash`, which maps objects to their position in the
heap.  This enforces a uniqueness constraint on items on the heap, and also
allows items to be more efficiently deleted or adjusted.  However maintaining
the hash does lead to a small drop in normal `#push` and `#pop` performance.

_TODO:_ Also ~~included is~~ _will include_ `DHeap::Lazy`, which contains some
features that are loosely inspired by go's timers.  e.g: It lazily sifts its
heap after deletion and adjustments, to achieve faster average runtime for *add*
and *cancel* operations.

## Development

After checking out the repo, run `bin/setup` to install dependencies. Then, run
`rake spec` to run the tests. You can also run `bin/console` for an interactive
prompt that will allow you to experiment.

To install this gem onto your local machine, run `bundle exec rake install`. To
release a new version, update the version number in `version.rb`, and then run
`bundle exec rake release`, which will create a git tag for the version, push
git commits and tags, and push the `.gem` file to
[rubygems.org](https://rubygems.org).

## Contributing

Bug reports and pull requests are welcome on GitHub at
https://github.com/nevans/d_heap. This project is intended to be a safe,
welcoming space for collaboration, and contributors are expected to adhere to
the [code of
conduct](https://github.com/nevans/d_heap/blob/master/CODE_OF_CONDUCT.md).

## License

The gem is available as open source under the terms of the [MIT
License](https://opensource.org/licenses/MIT).

## Code of Conduct

Everyone interacting in the DHeap project's codebases, issue trackers, chat
rooms and mailing lists is expected to follow the [code of
conduct](https://github.com/nevans/d_heap/blob/master/CODE_OF_CONDUCT.md).
