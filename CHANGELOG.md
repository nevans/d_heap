## Current/Unreleased

## Release v0.4.0 (2021-01-12)

* ⚡️ Big performance improvements, by using C `long double *cscores` array
* ⚡️ Scores must be `Integer` in `-uint64..+uint64`, or convertable to `Float`
* ⚡️ many many (so many) updates to benchmarks
* ✨ Added `DHeap#clear`
* 🐛 Fixed `DHeap#initialize_copy` and `#freeze`
* ♻️  significant refactoring
* 📝 Updated docs (mostly adding benchmarks)

## Release v0.3.0 (2020-12-29)

* ⚡️ Big performance improvements, by converting to a `T_DATA` struct.
* ♻️  Major refactoring/rewriting of dheap.c
* ✅ Added benchmark specs
* 🔥 Removed class methods that operated directly on an array.  They weren't
    compatible with the performance improvements.

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
