# Benchmarks after switching from long double to double (v0.6.1-13-g1770484)

Previously SCORE used `long double` so it could store any `long int` without
loss of precision (which is necessary for storing nanoseconds since the epoch).
On the other hand, that can usually be worked around: e.g. monotonic clock
under linux gives nanoseconds since last boot, and nanosecond precision isn't
*that* important anyway.  By shortening `SCORE` from 80-bit (stored as 128-bit
on my system) to 64-bit, `ENTRY` is also shortened from 256-bit to 128-bit!
That's half the memory usage of before!

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
