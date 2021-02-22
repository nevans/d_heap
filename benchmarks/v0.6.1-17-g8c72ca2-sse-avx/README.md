# Benchmarks after converting to use SSE/AVX/AVX512 v0.6.1-21-g7bc7ca8

This was an experiment to see if searching for the min index via `<immintrin.h>`
vector extensions would be faster than a simple `for` loop (after compiler
optimizations and CPU pipelines' branch prediction).

The results were mixed: It is a speedup in some cases, especially larger `d`
values or larger heaps.  The `push_n_pop_n` benchmark runs as fast or faster
across the board.  But the `push_pop` benchmark _seemed_ to indicate it was a
significant slowdown for most `d` values and on "normal" sized heaps.

My current hypothesis is that `sift-down` and the `push_pop` benchmark both
suffer from non-random biases based on the `n % d`.  Because that benchmark
always maintains the exact same two sizes--pushing onto a size `n` heap and
popping from a size `n + 1` heap--it leads to biases for specific `d` values.
The benchmark should be fixed by introducing some "jitter" that will occasional
add an extra `push` or `pop` to test the heap sizes between and `n - d/2` up to
`n + d/2` thus hitting every single value of `n % d`.

One of the issues may be that, at the volumes were talking about, there are big
slowdowns just to add a single new `if` statement to the `sift-down` operation,
and a generic `d`-ary heap that allows any value for `d` needs to add multiple
new branches just to deal with large range of possible child counts.  With
macros or code generation, we could create a special `sift-down` for every
useful `d` value and use a branch-free "happy path" when `d` children are
present.  But that's another hypothesis to experiment with on another day.

In any case, it's an enormous jump in code complexity for speedups that are not
always present.  So it's kept as an experiment until it is _always_ faster.

Experiments TODO:
 * can inline methods attain the same speedup as macros?
 * does "jitter" make `push_pop` match `push_n_pop_n` more closely?
 * generate an unrolled version for every `d` up to 16 (and one for >16)

```
screenfetch -N -d '-de,wm,wmtheme,gtk'

                          ./+o+-       nick@anarres
                  yyyyy- -yyyyyy+      OS: Ubuntu 20.10 groovy
               ://+//////-yyyyyyo      Kernel: x86_64 Linux 5.8.0-43-generic
           .++ .:/++++++/-.+sss/`      Uptime: 12d 12h 52m
         .:++o:  /++++++++/:--:/-      Packages: 2359
        o:+o+:++.`..```.-/oo+++++/     Shell: bash 5.0.17
       .:+o:+o/.          `+sssoo+/    Resolution: 5760x2160
  .++/+:+oo+o:`             /sssooo.   Disk: 80G / 226G (38%)
 /+++//+:`oo+o               /::--:.   CPU: Intel Core i7-1065G7 @ 8x 3.9GHz [57.0°C]
 \+/+o+++`o++o               ++////.   GPU: Mesa Intel(R) Iris(R) Plus Graphics (ICL GT2)
  .++.o+++oo+:`             /dddhhh.   RAM: 19309MiB / 31694MiB
       .+.o+oo:.          `oddhhhh+
        \+.++o+o``-````.:ohdhhhhh+
         `:o+++ `ohhhhhhhhyo++os:
           .o:`.syhhhhhhh/.oo++o`
               /osyyyyyyo++ooo+++/
                   ````` +oo+++o\:
                          `oo++.
```
