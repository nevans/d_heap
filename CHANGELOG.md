## Current/Unreleased

* ✨ Added `DHeap::Map` for ensuring values can only be added once, by `#hash`.
    * Adding again will update the score.
    * Adds `DHeap::Map#[]` for quick lookup of existing scores
    * Adds `DHeap::Map#[]=` for adjustments of existing scores

## Release v0.6.1 (2021-01-24)

* 📝 Fix link to CHANGELOG.md in gemspec

## Release v0.6.0 (2021-01-24)

* 🔥 **Breaking**: `#initialize` uses a keyword argument for `d`
* ✨ Added `#initialize(capacity: capa)` to set initial capacity.
* ✨ Added `peek_with_score` and `peek_score`
* ✨ Added `pop_with_score` and `each_pop(with_score: true)`
* ✨ Added `pop_all_below(max_score, array = [])`
* ✨ Added aliases for `shift` and `next`
* 📈 Added benchmark charts to README, and `bin/bench_charts` to generate them.
    * requires `gruff` which requires `rmagick` which requires `imagemagick`
* 📝 Many documentation updates and fixes.

## Release v0.5.0 (2021-01-17)

* 🔥 **Breaking**: reversed order of `#push` arguments to `value, score`.
* ✨ Added `#insert(score, value)` to replace earlier version of `#push`.
* ✨ Added `#each_pop` enumerator.
* ✨ Added aliases for `deq`, `enq`, `first`, `pop_below`, `length`, and
    `count`, to mimic other classes in ruby's stdlib.
* ⚡️♻️  More performance improvements:
    * Created an `ENTRY` struct and store both the score and the value pointer in
      the same `ENTRY *entries` array.
    * Reduced unnecessary allocations or copies in both sift loops.  A similar
      refactoring also sped up the pure ruby benchmark implementation.
    * Compiling with `-O3`.
* 📝 Updated (and in some cases, fixed) yardoc
* ♻️  Moved aliases and less performance sensitive code into ruby.
* ♻️  DRY up push/insert methods

## Release v0.4.0 (2021-01-12)

* 🔥 **Breaking**: Scores must be `Integer` or convertable to `Float`
    * ⚠️  `Integer` scores must fit in `-ULONG_LONG_MAX` to `+ULONG_LONG_MAX`.
* ⚡️ Big performance improvements, by using C `long double *cscores` array
* ⚡️ many many (so many) updates to benchmarks
* ✨ Added `DHeap#clear`
* 🐛 Fixed `DHeap#initialize_copy` and `#freeze`
* ♻️  significant refactoring
* 📝 Updated docs (mostly adding benchmarks)

## Release v0.3.0 (2020-12-29)

* 🔥 **Breaking**: Removed class methods that operated directly on an array.
    They weren't compatible with the performance improvements.
* ⚡️ Big performance improvements, by converting to a `T_DATA` struct.
* ♻️  Major refactoring/rewriting of dheap.c
* ✅ Added benchmark specs

## Release v0.2.2 (2020-12-27)

* 🐛 fix `optimized_cmp`, avoiding internal symbols
* 📝 Update documentation
* 💚 fix macos CI
* ➕ Add rubocop 👮🎨

## Release v0.2.1 (2020-12-26)

* ⬆️  Upgraded rake (and bundler) to support ruby 3.0

## Release v0.2.0 (2020-12-24)

* ✨ Add ability to push separate score and value
* ⚡️ Big performance gain, by storing scores separately and using ruby's
  internal `OPTIMIZED_CMP` instead of always directly calling `<=>`

## Release v0.1.0 (2020-12-22)

🎉 initial release 🎉

* ✨ Add basic d-ary Heap implementation
