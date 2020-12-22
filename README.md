# DHeap

A fast _d_-ary heap implementation for ruby, useful in priority queues and graph
algorithms.

The _d_-ary heap data structure is a generalization of the binary heap, in which
the nodes have _d_ children instead of 2.  This allows for "decrease priority"
operations to be performed more quickly with the tradeoff of slower delete
minimum.  Additionally, _d_-ary heaps can have better memory cache behavior than
binary heaps, allowing them to run more quickly in practice despite slower
worst-case time complexity.

_TODO:_ In addition to a basic _d_-ary heap class (`DHeap`), this library
~~includes~~ _will include_ extensions to `Array`, allowing an Array to be
directly handled as a priority queue.  These extension methods are meant to be
used similarly to how `#bsearch` and `#bsearch_index` might be used.

_TODO:_ Also included is `DHeap::Set`, which augments the basic heap with an
internal `Hash`, which maps a set of values to scores.
loosely inspired by go's timers.  e.g: It lazily sifts its heap after deletion
and adjustments, to achieve faster average runtime for *add* and *cancel*
operations.

_TODO:_ Also included is `DHeap::Timers`, which contains some features that are
loosely inspired by go's timers.  e.g: It lazily sifts its heap after deletion
and adjustments, to achieve faster average runtime for *add* and *cancel*
operations.

## Motivation

Ruby's Array class comes with some helpful methods for maintaining a sorted
array, by combining `#bsearch_index` with `#insert`.  With certain insert/remove
workloads that can perform very well, but in the worst-case an insert or delete
can result in O(n), since it may need to memcopy a significant portion of the
array.  Knowing that priority queues are usually implemented with a heap, and
that the heap is a relatively simple data structure, I set out to replace my
`#bsearch_index` and `#insert` code with a one.  I was surprised to find that,
at least under certain benchmarks, my ruby Heap implementation was tied with or
slower than inserting into a fully sorted array.  On the one hand, this is a
testament to ruby's fine-tuned Array implementation.  On the other hand, it
seemed like a heap implementated in C should easily match the speed of ruby's
bsearch + insert.

Additionally, I was inspired by reading go's "timer.go" implementation to
experiment with a 4-ary heap, instead of the traditional binary heap.  In the
case of timers, new timers are usually scheduled to run after most of the
existing timers and timers are usually canceled before they have a chance to
run. While a binary heap holds 50% of its elements in its last layer, 75% of a
4-ary heap will have no children.  That diminishes the extra comparison
overhead during sift-down.

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

The simplest way to use it is simply with `#push` and `#pop`.  Push will 

```ruby
require "d_heap"

heap = DHeap.new # defaults to a 4-ary heap

# storing [time, task] tuples
heap << [Time.now + 5*60, Task.new(1)]
heap << [Time.now +   30, Task.new(2)]
heap << [Time.now +   60, Task.new(3)]
heap << [Time.now +    5, Task.new(4)]

# peeking and popping (using last to get the task and ignore the time)
heap.pop.last # => Task[4]
heap.pop.last # => Task[2]
heap.peak.last # => Task[3]
heap.pop.last # => Task[3]
heap.pop.last # => Task[1]
```

Read the `rdoc` for more detailed documentation and examples.

## Benchmarks

_TODO: put benchmarks here._

## Alternative data structures

Depending on what you're doing, maintaining a sorted `Array` using
`#bsearch_index` and `#insert` might be faster!

If it is important to be able to quickly enumerate the set or find the ranking
of values in it, then you probably want to use a self-balancing binary search
tree (e.g. a red-black tree) or a skip-list.

A Hashed Timing Wheel or Heirarchical Timing Wheels (or some variant in that
family of data structures) can be constructed to have effectively O(1) running
time in most cases.  However, the implementation for that data structure is much
more complex than a heap.  If a 4-ary heap is good enough for go's timers,
it should be suitable for many use cases.

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
