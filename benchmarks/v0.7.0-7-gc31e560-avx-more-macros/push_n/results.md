# Iteration per second (i/s)

|                  |N 300_000|N 100_000|N 30_000|N 10_000| N 3_000| N 1_000|   N 300|   N 100|    N 30|    N 10|
|:-----------------|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push N (findmin)  |  11.771M|  11.866M| 12.033M| 12.117M| 12.014M| 11.962M| 11.850M| 11.626M| 11.725M| 11.321M|
|push N (bsearch)  |  64.509k| 229.477k|637.707k|  1.216M|  1.694M|  2.024M|  2.443M|  2.932M|  3.634M|  4.448M|
|push N (rb_heap)  |   4.582M|   4.644M|  4.613M|  4.675M|  4.671M|  4.674M|  4.715M|  4.825M|  5.254M|  5.792M|
|push N (c++ stl)  |   8.472M|   8.695M|  8.812M|  9.198M|  9.128M|  9.085M|  9.007M|  8.939M|  8.304M|  7.107M|
|push N (c_dheap)  |  13.081M|  13.098M| 13.027M| 13.236M| 13.095M| 13.040M| 12.951M| 13.033M| 13.031M| 12.422M|