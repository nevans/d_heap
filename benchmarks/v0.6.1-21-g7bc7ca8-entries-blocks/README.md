# Benchmarks after converting to "entries_blocks" v0.6.1-21-g7bc7ca8

This was an experiment to see if switching from `_mm512_set_pd` to
`_mm512_loadu_pd` to (and similar for 256-bit and 128-bit vectors) would speed
up the SSE/AVX2/AVX512 vector based code.  The results were mixed, but mostly
very bad.

I suppose the speed of assigning a single struct for each entry outweighs the
speed of loading a vector from a single address instead of multiple.

On the other hand, given the nature of these things, it could be that I
accidentally enabled a debug assertion in the benchmark version! (I did.)

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
