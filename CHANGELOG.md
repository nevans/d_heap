## Current/Unreleased

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

* ⚡️ Big performance improvements, by using C `long double *cscores` array
* ⚡️ Scores must be `Integer` in `-uint64..+uint64`, or convertable to `Float`
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
