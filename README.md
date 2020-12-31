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
heap.peek.last # => Task[3], but don't pop it
heap.pop.last  # => Task[3]
heap.pop.last  # => Task[1]
```

Read the `rdoc` for more detailed documentation and examples.

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
array.  While this is a testament to ruby's fine-tuned Array implementation, a
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

## Benchmarks

These benchmarks were measured with v0.3.0.  They use simple implementations for
a pure-ruby heap, calling `Array#sort` after each insert, and calling
`Array#insert` after a binary search.  The benchmark implementations don't allow
keeping scores and values seperately, which would lead to slowdowns if they
needed to call `a <=> b` for comparisons.  To rule that out, I've used only
integer (`Fixnum`) values, which should compare faster than any other object
type.  I tried to tune these example implementations so that the comparisons
would be fair.

For very small N values all implementations are roughly equal, although the pure
ruby binary heap does start to lag:

     push and resort [n=3]:   333884.8 i/s
    bsearch + insert [n=3]:   329975.9 i/s - same-ish: difference falls within error
    quaternary DHeap [n=3]:   312167.2 i/s - same-ish: difference falls within error
    ruby binary heap [n=3]:   291335.9 i/s - same-ish: difference falls within error

With these random pushes + pops, DHeap does run faster than bsearch + insert
once N > ~5, but the difference is still very small even up to N=31.

    quaternary DHeap [n=31]:    74543.2 i/s
    bsearch + insert [n=31]:    61674.5 i/s - same-ish: difference falls within error
     push and resort [n=31]:    42972.6 i/s - 1.73x  (± 0.00) slower
    ruby binary heap [n=31]:    28067.5 i/s - 2.66x  (± 0.00) slower

OOnce you get to N=100 though, the difference is very measurable, and it only
gets wider as N increases.  Surprisingly (or at least, it surprised _me_ at
first), my bsearch + insert still beats my pure ruby binary heap at N=10k:

    quaternary DHeap [n=100]:    23997.9 i/s
    bsearch + insert [n=100]:    17300.5 i/s - 1.39x  (± 0.00) slower
    ruby binary heap [n=100]:     6851.2 i/s - 3.50x  (± 0.00) slower
     push and resort [n=100]:     5859.6 i/s - 4.10x  (± 0.00) slower

    quaternary DHeap [n=10000]:      164.9 i/s
    bsearch + insert [n=10000]:       81.0 i/s - 2.04x  (± 0.00) slower
    ruby binary heap [n=10000]:       36.6 i/s - 4.50x  (± 0.00) slower
     push and resort [n=10000]:        0.5 i/s - 352.10x  (± 0.00) slower

By N=100k, the O(log n) vs O(n) complexity finally started to show, and my ruby
binary heap does pull ahead of my bsearch + index:

    quaternary DHeap [n=100000]:       13.4 i/s
    ruby binary heap [n=100000]:        3.0 i/s - 4.55x  (± 0.00) slower
    bsearch + insert [n=100000]:        1.6 i/s - 8.45x  (± 0.00) slower
     push and resort [n=100000]: ****** not run... I'm too impatient ******

It's interesting to look at the `ruby-prof` results for a simplistic binary
search + insert implementation, pushing 50k random integers, then popping all
of them. In particular, even with 50k members, the linear `Array#insert` is
_still_ faster than the logarithmic `Array#bsearch_index`. At this scale, ruby
comparisons are still (relatively) slow and `memcpy` is (relatively) quite fast!

    %self      total      self      wait     child     calls  name                           location
    45.23      1.664     1.664     0.000     0.000    50000   Array#bsearch_index
    17.58      2.585     0.647     0.000     1.938    50000   BinarySearchAndInsert#<<       bin/benchmark_implementations.rb:55
    14.82      3.678     0.545     0.000     3.133        1   Object#run_in_and_out          bin/benchmarks:33
     8.90      0.438     0.327     0.000     0.111    50001   BinarySearchAndInsert#pop      bin/benchmark_implementations.rb:64
     7.47      0.275     0.275     0.000     0.000    50000   Array#insert
     6.00      0.221     0.221     0.000     0.000   100001   Array#pop

_n.b. 50k of those Array#pop calls are for the unsorted input array_

Keep in mind: RubyProf impacts these different implementations differently.  I
wouldn't use the RubyProf results for comparisons between implementations.
Contrast this with a simplistic pure-ruby implementation of a binary heap:

    %self      total      self      wait     child     calls  name                           location
    79.72     11.339    10.996     0.000     0.342    50001   NaiveBinaryHeap#pop            bin/benchmark_implementations.rb:90
    12.08      1.779     1.667     0.000     0.112    50000   NaiveBinaryHeap#<<             bin/benchmark_implementations.rb:74
     4.09     13.794     0.564     0.000    13.230        1   Object#run_in_and_out          bin/benchmarks:33
     1.65      0.227     0.227     0.000     0.000   100000   Array#pop

You can see that it spends almost 7x more time in pop than it does in push.
That is expected behavior for a heap: although both are O(log n), pop is
significantly more complex, and has _d_ comparisons per layer.

See `bin/benchmarks` and `docs/benchmarks.txt` for more details.

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

## TODOs...

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
