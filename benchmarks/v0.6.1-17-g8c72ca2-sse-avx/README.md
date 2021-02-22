# Benchmarks after converting to use SSE/AVX/AVX512 v0.6.1-21-g7bc7ca8

This was an experiment to see if searching for the min index via `<immintrin.h>`
vector extensions would be faster than a simple `for` loop (after compiler
optimizations and CPU pipelines' branch prediction).  The results were mixed: It
is a speedup for larger `d` values especially on larger heaps.  Unfortunately
it was a significant slowdown for the ideal `d` values on "normal" sized heaps.
And in any case, it was an enormous jump in code complexity for speedups that
were only apparent on very _very_ large heaps (e.g. ~10 million entries).

One of the issues may be that, at the volumes were talking about, there are big
slowdowns just to add a single new `if` statement to the `sift-down` operation,
and a generic `d`-ary heap that allows any value for `d` needs to add multiple
new branches just to deal with large range of possible child counts.  With
macros or code generation, we could create a special `sift-down` for every
useful `d` value and use a branch-free "happy path" when `d` children are
present.  But that's another hypothesis to experiment with on another day.

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
