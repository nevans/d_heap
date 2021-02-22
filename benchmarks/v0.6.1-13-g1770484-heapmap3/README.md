# Benchmarks after adding DHeap::Map (v0.6.1-13-g1770484)

n.b. more benchmarks should be added for `DHeap::Map` in other scenarios.

There is definitely room for improvements on the speed of this, but my first
experiment (using `st_init_numtable` directly) was a huge failure.

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
