# DHeap

A fast _d_-ary heap implementation for ruby, useful in priority queues and graph
algorithms.

The _d_-ary heap data structure is a generalization of the binary heap, in which
the nodes have _d_ children instead of 2.  This allows for "decrease priority"
operations to be performed more quickly with the tradeoff of slower delete
minimum.  Additionally, _d_-ary heaps can have better memory cache behavior than
binary heaps, allowing them to run more quickly in practice despite slower
worst-case time complexity. In the worst case, a _d_-ary heap requires only
`O(log n / log d)` to push, with the tradeoff that pop is `O(d log n / log d)`.

Although you should probably just stick with the default _d_ value  of `4`, it
may be worthwhile to benchmark your specific scenario.

## Motivation

Sometimes you just need a priority queue, right?  With a regular queue, you
expect "FIFO" behavior: first in, first out.  With a priority queue, you push
with a score (or your elements are comparable), and you want to be able to
efficiently pop off the minimum (or maximum) element.

One obvious approach is to simply maintain an array in sorted order.  And
ruby's Array class makes it simple to maintain a sorted array by combining
`#bsearch_index` with `#insert`.  With certain insert/remove workloads that can
perform very well, but in the worst-case an insert or delete can result in O(n),
since `#insert` may need to `memcpy` or `memmove` a significant portion of the
array.

But the standard way to efficiently and simply solve this problem is using a
binary heap.  Although it increases the time for `pop`, it converts the
amortized time per push + pop from `O(n)` to `O(d log n / log d)`.

I was surprised to find that, at least under certain benchmarks, my pure ruby
heap implementation was usually slower than inserting into a fully sorted
array.  While this is a testament to ruby's fine-tuned Array implementationw, a
heap implementated in C should easily peform faster than `Array#insert`.

The biggest issue is that it just takes far too much time to call `<=>` from
ruby code: A sorted array only requires `log n / log 2` comparisons to insert
and no comparisons to pop.  However a _d_-ary heap requires `log n / log d` to
insert plus an additional `d log n / log d` to pop.  If your queue contains only
a few hundred items at once, the overhead of those extra calls to `<=>` is far
more than occasionally calling `memcpy`.

It's likely that MJIT will eventually make the C-extension completely
unnecessary.  This is definitely hotspot code, and the basic ruby implementation
would work fine, if not for that `<=>` overhead.  Until then... this gem gets
the job done.

## Installation

Add this line to your application's Gemfile:

```ruby
gem 'd_heap'
```

And then execute:

    $ bundle install

Or install it yourself as:

    $ gem install d_heap

## Usage

The simplest way to use it is simply with `#push` and `#pop`.  Push takes a
score and a value, and pop returns the value with the current minimum score.

```ruby
require "d_heap"

heap = DHeap.new # defaults to a 4-ary heap

# storing [score, value] tuples
heap.push Time.now + 5*60, Task.new(1)
heap.push Time.now +   30, Task.new(2)
heap.push Time.now +   60, Task.new(3)
heap.push Time.now +    5, Task.new(4)

# peeking and popping (using last to get the task and ignore the time)
heap.pop.last  # => Task[4]
heap.pop.last  # => Task[2]
heap.peak.last # => Task[3], but don't pop it
heap.pop.last  # => Task[3]
heap.pop.last  # => Task[1]
```

Read the `rdoc` for more detailed documentation and examples.

## TODOs...

_TODO:_ In addition to a basic _d_-ary heap class (`DHeap`), this library
~~includes~~ _will include_ extensions to `Array`, allowing an Array to be
directly handled as a priority queue.  These extension methods are meant to be
used similarly to how `#bsearch` and `#bsearch_index` might be used.

_TODO:_ Also ~~included is~~ _will include_ `DHeap::Set`, which augments the
basic heap with an internal `Hash`, which maps a set of values to scores.
loosely inspired by go's timers.  e.g: It lazily sifts its heap after deletion
and adjustments, to achieve faster average runtime for *add* and *cancel*
operations.

_TODO:_ Also ~~included is~~ _will include_ `DHeap::Timers`, which contains some
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

## Benchmarks

_TODO: put benchmarks here._

## Analysis

### Time complexity

Both sift operations can perform (log[d] n = log n / log d) swaps.
Swap up performs only a single comparison per swap: O(1).
Swap down performs as many as d comparions per swap: O(d).

Inserting an item is O(log n / log d).
Deleting the root is O(d log n / log d).

Assuming every inserted item is eventually deleted from the root, d=4 requires
the fewest comparisons for combined insert and delete:
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

Because the heap is a complete binary tree, space usage is linear, regardless
of d.  However higher d values may provide better cache locality.

We can run comparisons much much faster for Numeric or String objects than for
ruby objects which delegate comparison to internal Numeric or String objects.
And it is often advantageous to use extrinsic scores for uncomparable items.
For this, our internal array uses twice as many entries (one for score and one
for value) as it would if it only supported intrinsic comparison or used an
un-memoized "sort_by" proc.

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

Depending on what you're doing, maintaining a sorted `Array` using
`#bsearch_index` and `#insert` might be faster!  Although it is technically
O(n) for insertions, the implementations for `memcpy` or `memmove` can be *very*
fast on modern architectures.  Also, it can be faster O(n) on average, if
insertions are usually near the end of the array.  You should run benchmarks
with your expected scenarios to determine which is right.

If it is important to be able to quickly enumerate the set or find the ranking
of values in it, then you probably want to use a self-balancing binary search
tree (e.g. a red-black tree) or a skip-list.

A Hashed Timing Wheel or Heirarchical Timing Wheels (or some variant in that
family of data structures) can be constructed to have effectively O(1) running
time in most cases.  However, the implementation for that data structure is more
complex than a heap.  If a 4-ary heap is good enough for go's timers, it should
be suitable for many use cases.

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
