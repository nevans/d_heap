## Benchmark comparison implementations

 * **findmin** -
    A very fast `O(1)` push using `Array#push` onto an unsorted Array, but a
    very slow `O(n)` pop using `Array#min`, `Array#rindex(min)` and
    `Array#delete_at(min_index)`.  Push + pop is still fast for `n < 100`, but
    unusably slow for `n > 1000`.

 * **bsearch** -
    A simple implementation with a slow `O(n)` push using `Array#bsearch` +
    `Array#insert` to maintain a sorted Array, but a very fast `O(1)` pop with
    `Array#pop`.  It is still relatively fast for `n < 10000`, but its linear
    time complexity really destroys it after that.

 * **rb_heap** -
    A pure ruby binary min-heap that has been tuned for performance by making
    few method calls and allocating and assigning as few variables as possible.
    It runs in `O(log n)` for both push and pop, although pop is slower than
    push by a constant factor.  Its much higher constant factors makes it lose
    to `bsearch` push + pop for `n < 10000` but it holds steady with very little
    slowdown even with `n > 10000000`.

 * **c++ stl** -
    A thin wrapper around the [priority_queue_cxx gem] which uses the [C++ STL
    priority_queue].  The wrapper is simply to provide compatibility with the
    other benchmarked implementations, but it should be possible to speed this
    up a little bit by benchmarking the `priority_queue_cxx` API directly.  It
    has the same time complexity as rb_heap but its much lower constant
    factors allow it to easily outperform `bsearch`.

 * **c_dheap** -
    A {DHeap} instance with the default `d` value.  It has the same time
    complexity as `rb_heap` and `c++ stl`, but is faster than both in every
    benchmarked scenario.

[priority_queue_cxx gem]: https://rubygems.org/gems/priority_queue_cxx
[C++ STL priority_queue]: http://www.cplusplus.com/reference/queue/priority_queue/
