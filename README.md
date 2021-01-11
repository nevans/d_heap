# DHeap

A fast [_d_-ary heap][d-ary heap] [priority queue] implementation for ruby,
implemented as a C extension.

With a regular queue, you expect "FIFO" behavior: first in, first out.  With a
stack you expect "LIFO": last in first out.  With a priority queue, you push
elements along with a score and the lowest scored element is the first to be
poppsed.  Priority queues are often used in algorithms for e.g. [scheduling] of
timers or bandwidth management, [Huffman coding], and various graph search
algorithms such as [Dijkstra's algorithm], [A* search], or [Prim's algorithm].

The _d_-ary heap data structure is a generalization of the [binary heap], in
which the nodes have _d_ children instead of 2.  This allows for "decrease
priority" operations to be performed more quickly with the tradeoff of slower
delete minimum.  Additionally, _d_-ary heaps can have better memory cache
behavior than binary heaps, allowing them to run more quickly in practice
despite slower worst-case time complexity. In the worst case, a _d_-ary heap
requires only `O(log n / log d)` to push, with the tradeoff that pop is `O(d log
n / log d)`.

Although you should probably just use the default _d_ value  of `4` (see the
analysis below), it's always advisable to benchmark your specific use-case.

[d-ary heap]: https://en.wikipedia.org/wiki/D-ary_heap
[priority queue]: https://en.wikipedia.org/wiki/Priority_queue
[binary heap]: https://en.wikipedia.org/wiki/Binary_heap
[scheduling]: https://en.wikipedia.org/wiki/Scheduling_(computing)
[Huffman coding]: https://en.wikipedia.org/wiki/Huffman_coding#Compression
[Dijkstra's algorithm]: https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm#Using_a_priority_queue
[A* search]: https://en.wikipedia.org/wiki/A*_search_algorithm#Description
[Prim's algorithm]: https://en.wikipedia.org/wiki/Prim%27s_algorithm

## Usage

The basic interface is `#push(score, value)` to adds the value ranked by the
score, and `#pop` to remove and return the value with the minimum score.

The score must be Numeric or convertable to a `Float` via `Float(score)`.  _n.b.
Because of the enormous performance impact, the score must either be an integer
with an absolute value that fits into_ `unsigned long long` _or a_ `double`
_(architecture dependant, but these are both 64bit values on an IA-64 system).
This gives a 40+% speedup under some simulations!  Comparing arbitary objects
via_ `a <=> b` _was the original design and may be added back in a future
version,_ iff _it can be done without impacting the speed of numeric
comparisons._

```ruby
require "d_heap"

heap = DHeap.new # defaults to a 4-heap

# storing [score, value] tuples
heap.push Time.now + 5*60, Task.new(1)
heap.push Time.now +   30, Task.new(2)
heap.push Time.now +   60, Task.new(3)
heap.push Time.now +    5, Task.new(4)

# peeking and popping (using last to get the task and ignore the time)
heap.pop    # => Task[4]
heap.pop    # => Task[2]
heap.peek   # => Task[3], but don't pop it from the heap
heap.pop    # => Task[3]
heap.pop    # => Task[1]
heap.empty? # => true
heap.pop    # => nil
```

If your values behave as their own score, by being convertible via
`Float(value)`, then you can use `#<<` for implicit scoring.  The score should
not change for as long as the value remains in the heap, since it will not be
re-evaluated after being pushed.

```ruby
heap.clear

# The score can be derived from the value by using to_f.
# "a <=> b" is *much* slower than comparing numbers, so it isn't used.
class Event
  include Comparable
  attr_reader :time, :payload
  alias_method :to_time, :time

  def initialize(time, payload)
    @time = time.to_time
    @payload = payload
    freeze
  end

  def to_f
    time.to_f
  end

  def <=>(other)
    to_f <=> other.to_f
  end
end

heap << comparable_max # sorts last, using <=>
heap << comparable_min # sorts first, using <=>
heap << comparable_mid # sorts in the middle, using <=>
heap.pop    # => comparable_min
heap.pop    # => comparable_mid
heap.pop    # => comparable_max
heap.empty? # => true
heap.pop    # => nil
```

You can also pass a value into `#pop(max)` which will only pop if the minimum
score is less than or equal to `max`.

Read the [API documentation] for more detailed documentation and examples.

[API documentation]: https://rubydoc.info/gems/d_heap/DHeap

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
This can be very simply implemented using `Array#bseach_index` + `Array#insert`.
This can be very fast—`Array#pop` is `O(1)`—but the worst-case for insert is
`O(n)` because it may need to `memcpy` a significant portion of the array.

The standard way to implement a priority queue is with a binary heap.  Although
this increases the time for `pop`, it converts the amortized time per push + pop
from `O(n)` to `O(d log n / log d)`.

However, I was surprised to find that—at least for some benchmarks—my pure ruby
heap implementation was much slower than inserting into and popping from a fully
sorted array.  The reason for this surprising result: Although it is `O(n)`,
`memcpy` has a _very_ small constant factor, and calling `<=>` from ruby code
has relatively _much_ larger constant factors.  If your queue contains only a
few thousand items, the overhead of those extra calls to `<=>` is _far_ more
than occasionally calling `memcpy`.  In the worst case, a _d_-heap will require
`d + 1` times more comparisons for each push + pop than a `bsearch` + `insert`
sorted array.

Moving the sift-up and sift-down code into C helps some.  But much more helpful
is optimizing the comparison of numeric scores, so `a <=> b` never needs to be
called.  I'm hopeful that MJIT will eventually obsolete this C-extension.  JRuby
or TruffleRuby may already run the pure ruby version at high speed.  This can be
hotspot code, and the basic ruby implementation should perform well if not for
the high overhead of `<=>`.

## Analysis

### Time complexity

There are two fundamental heap operations: sift-up (used by push) and sift-down
(used by pop).

* Both sift operations can perform as many as `log n / log d` swaps, as the
  element may sift from the bottom of the tree to the top, or vice versa.
* Sift-up performs a single comparison per swap: `O(1)`.
  So pushing a new element is `O(log n / log d)`.
* Swap down performs as many as d comparions per swap: `O(d)`.
  So popping the min element is `O(d log n / log d)`.

Assuming every inserted element is eventually deleted from the root, d=4
requires the fewest comparisons for combined insert and delete:

* (1 + 2) lg 2 = 4.328085
* (1 + 3) lg 3 = 3.640957
* (1 + 4) lg 4 = 3.606738
* (1 + 5) lg 5 = 3.728010
* (1 + 6) lg 6 = 3.906774
* etc...

Leaf nodes require no comparisons to shift down, and higher values for d have
higher percentage of leaf nodes:

* d=2 has ~50% leaves,
* d=3 has ~67% leaves,
* d=4 has ~75% leaves,
* and so on...

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

## Benchmarks

_See `bin/benchmarks` and `docs/benchmarks.txt`, as well as `bin/profile` and
`docs/profile.txt` for more details or updated results. These benchmarks were
measured with v0.3.0 and ruby 2.7.2 without MJIT enabled._

These benchmarks use very simple implementations for a pure-ruby heap and an
array that is kept sorted using `Array#bsearch_index` and `Array#insert`.  For
comparison, an alternate implementation using `Array#sort` after each insert is
also shown.  The benchmark implementations do not use separate scores and
values, which allows the example implementations to use half as much memory as
`DHeap` and as a result may give them a small comparative performance boost.
To measure the lowest possible comparison overhead, the benchmarks use only
integer (`Fixnum`) scores, which compare faster than any other object type.
Because `DHeap` specifically optimizes for `Integer` and `Float` scores, other
score types would likely perform more slowly.

Three different scenarios are measured:
 * push N values but never pop (clearing between each set of pushes).
 * push N values and then pop N values.
   Although this could be used for heap sort, we're unlikely to choose heap sort
   over Ruby's quick sort implementation. I'm using this scenario to represent
   the amortized cost of creating a heap and (eventually) draining it.
 * For a heap of size N, repeatedly push and pop while keeping a stable size.
   This is a _very simple_ approximation for how most scheduler/timer heaps
   would be used. Usually when a timer fires it will be quickly replaced by a
   new timer, and the overall count of timers will remain roughly stable.

In these benchmarks, `DHeap` runs faster than all other implementations for
every scenario and every value of N, although the difference is much more
noticable at higher values of N.  The pure ruby heap implementation is
competitive for `push` alone at every value of N, but is significantly slower
than bsearch + insert for push + pop until N is _very_ large (somewhere between
10k and 100k)!

For very small N values the benchmark implementations,  `DHeap` runs faster than
the other implementations for each scenario, although the difference is still
relatively small.  The pure ruby binary heap is 2x or more slower than bsearch +
insert for common common push/pop scenario.

    == push N (N=5) ==========================================================
    push N (c_dheap):   1590118.0 i/s
    push N (rb_heap):    922644.3 i/s - 1.72x  slower
    push N (bsearch):    920035.0 i/s - 1.73x  slower
    push N (sorting):    819813.5 i/s - 1.94x  slower

    == push N then pop N (N=5) ===============================================
    push N + pop N (c_dheap):   1085752.1 i/s
    push N + pop N (bsearch):    765205.7 i/s - 1.42x  slower
    push N + pop N (sorting):    694043.4 i/s - 1.56x  slower
    push N + pop N (rb_heap):    465018.2 i/s - 2.33x  slower

    == Push/pop with pre-filled queue of size=N (N=5) ========================
    push + pop (c_dheap):   5411341.7 i/s
    push + pop (bsearch):   4259042.8 i/s - 1.27x  slower
    push + pop (sorting):   2852751.5 i/s - 1.90x  slower
    push + pop (rb_heap):   2186260.5 i/s - 2.48x  slower

By N=21, `DHeap` has pulled significantly ahead of bsearch + insert for all
scenarios, and the pure ruby heap has fallen behind every implementation—even
resorting the array after every `#push`—in any scenario that uses `#pop`.

    == push N (N=21) =========================================================
    push N (c_dheap):    389943.6 i/s
    push N (rb_heap):    207784.2 i/s - 1.88x  slower
    push N (bsearch):    172513.5 i/s - 2.26x  slower
    push N (sorting):    112125.6 i/s - 3.48x  slower

    == push N then pop N (N=21) ==============================================
    push N + pop N (c_dheap):    196376.1 i/s
    push N + pop N (bsearch):    141854.7 i/s - 1.38x  slower
    push N + pop N (sorting):     96815.7 i/s - 2.03x  slower
    push N + pop N (rb_heap):     71385.1 i/s - 2.75x  slower

    == Push/pop with pre-filled queue of size=N (N=21) =======================
    push + pop (c_dheap):   4721156.3 i/s
    push + pop (bsearch):   3316974.2 i/s - 1.42x  slower
    push + pop (rb_heap):   1574341.6 i/s - 3.00x  slower
    push + pop (sorting):   1449704.5 i/s - 3.26x  slower

At higher values of N, `DHeap`'s logarithmic growth leads to little slowdown
of `DHeap#push`, while insert's linear growth causes it to run slower and
slower.  But because `#pop` is O(1) for a sorted array and O(d log n / log d)
for a _d_-heap, scenarios involving `#pop` remain relatively close even as N
increases to 5k:

    == Push/pop with pre-filled queue of size=N (N=5461) ==============
    push + pop (c_dheap):   2939356.4 i/s
    push + pop (bsearch):   1950891.5 i/s - 1.51x  slower
    push + pop (rb_heap):    815817.2 i/s - 3.60x  slower
    push + pop (sorting):     25603.2 i/s - 114.80x  slower

Somewhat surprisingly, bsearch + insert still runs faster than a pure ruby heap
for the repeated push/pop scenario, all the way up to N as high as 87k:

    == push N (N=87381) ======================================================
    push N (c_dheap):        88.3 i/s
    push N (rb_heap):        43.6 i/s - 2.02x  slower
    push N (bsearch):         2.9 i/s - 30.34x  slower

    == push N then pop N (N=87381) ===========================================
    push N + pop (c_dheap):        21.8 i/s
    push N + pop (rb_heap):         5.4 i/s - 4.07x  slower
    push N + pop (bsearch):         2.8 i/s - 7.86x  slower

    == Push/pop with pre-filled queue of size=N (N=87381) ====================
    push + pop (c_dheap):   1765639.1 i/s
    push + pop (bsearch):    774272.3 i/s - 2.28x  slower
    push + pop (rb_heap):    523594.7 i/s - 3.37x  slower

## Profiling

_n.b. `Array#fetch` is reading the input data, external to heap operations.
These benchmarks use integers for all scores, which enables significantly faster
comparisons.  If `a <=> b` were used instead, then the difference between push
and pop would be much larger.  And ruby's `Tracepoint` impacts these different
implementations differently.  So we can't use these profiler results for
comparisons between implementations.  A sampling profiler would be needed for
more accurate relative measurements._

It's informative to look at the `ruby-prof` results for a simple binary search +
insert implementation, repeatedly pushing and popping to a large heap. In
particular, even with 1000 members, the linear `Array#insert` is _still_ faster
than the logarithmic `Array#bsearch_index`. At this scale, ruby comparisons are
still (relatively) slow and `memcpy` is (relatively) quite fast!

    %self      total      self      wait     child     calls  name                           location
    34.79      2.222     2.222     0.000     0.000  1000000   Array#insert
    32.59      2.081     2.081     0.000     0.000  1000000   Array#bsearch_index
    12.84      6.386     0.820     0.000     5.566        1   DHeap::Benchmarks::Scenarios#repeated_push_pop d_heap/benchmarks.rb:77
    10.38      4.966     0.663     0.000     4.303  1000000   DHeap::Benchmarks::BinarySearchAndInsert#<< d_heap/benchmarks/implementations.rb:61
     5.38      0.468     0.343     0.000     0.125  1000000   DHeap::Benchmarks::BinarySearchAndInsert#pop d_heap/benchmarks/implementations.rb:70
     2.06      0.132     0.132     0.000     0.000  1000000   Array#fetch
     1.95      0.125     0.125     0.000     0.000  1000000   Array#pop

Contrast this with a simplistic pure-ruby implementation of a binary heap:

    %self      total      self      wait     child     calls  name                           location
    48.52      8.487     8.118     0.000     0.369  1000000   DHeap::Benchmarks::NaiveBinaryHeap#pop d_heap/benchmarks/implementations.rb:96
    42.94      7.310     7.184     0.000     0.126  1000000   DHeap::Benchmarks::NaiveBinaryHeap#<< d_heap/benchmarks/implementations.rb:80
     4.80     16.732     0.803     0.000    15.929        1   DHeap::Benchmarks::Scenarios#repeated_push_pop d_heap/benchmarks.rb:77

You can see that it spends almost more time in pop than it does in push.  That
is expected behavior for a heap: although both are O(log n), pop is
significantly more complex, and has _d_ comparisons per layer.

And `DHeap` shows a similar comparison between push and pop, although it spends
half of its time in the benchmark code (which is written in ruby):

    %self      total      self      wait     child     calls  name                           location
    43.09      1.685     0.726     0.000     0.959        1   DHeap::Benchmarks::Scenarios#repeated_push_pop d_heap/benchmarks.rb:77
    26.05      0.439     0.439     0.000     0.000  1000000   DHeap#<<
    23.57      0.397     0.397     0.000     0.000  1000000   DHeap#pop
     7.29      0.123     0.123     0.000     0.000  1000000   Array#fetch

### Timers

Additionally, when used to sort timers, we can reasonably assume that:
 * New timers usually sort after most existing timers.
 * Most timers will be canceled before executing.
 * Canceled timers usually sort after most existing timers.

So, if we are able to delete an item without searching for it, by keeping a map
of positions within the heap, most timers can be inserted and deleted in O(1)
time.  Canceling a non-leaf timer can be further optimized by marking it as
canceled without immediately removing it from the heap.  If the timer is
rescheduled before we garbage collect, adjusting its position will usually be
faster than a delete and re-insert.

## Alternative data structures

As always, you should run benchmarks with your expected scenarios to determine
which is right.

Depending on what you're doing, maintaining a sorted `Array` using
`#bsearch_index` and `#insert` might be just fine!  As discussed above, although
it is `O(n)` for insertions, `memcpy` is so fast on modern hardware that this
may not matter.  Also, if you can arrange for insertions to occur near the end
of the array, that could significantly reduce the `memcpy` overhead even more.

More complex heap varients, e.g. [Fibonacci heap], can allow heaps to be merged
as well as lower amortized time.

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

_TODO:_ Also ~~included is~~ _will include_ `DHeap::Set`, which augments the
basic heap with an internal `Hash`, which maps a set of values to scores.
loosely inspired by go's timers.  e.g: It lazily sifts its heap after deletion
and adjustments, to achieve faster average runtime for *add* and *cancel*
operations.

_TODO:_ Also ~~included is~~ _will include_ `DHeap::Lazy`, which contains some
features that are loosely inspired by go's timers.  e.g: It lazily sifts its
heap after deletion and adjustments, to achieve faster average runtime for *add*
and *cancel* operations.

Additionally, I was inspired by reading go's "timer.go" implementation to
experiment with a 4-ary heap instead of the traditional binary heap.  In the
case of timers, new timers are usually scheduled to run after most of the
existing timers.  And timers are usually canceled before they have a chance to
run. While a binary heap holds 50% of its elements in its last layer, 75% of a
4-ary heap will have no children.  That diminishes the extra comparison overhead
during sift-down.

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
