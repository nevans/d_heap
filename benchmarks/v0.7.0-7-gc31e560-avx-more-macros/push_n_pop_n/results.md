# Iteration per second (i/s)

|                          |N 300_000|N 100_000|N 30_000|N 10_000| N 3_000| N 1_000|   N 300|   N 100|    N 30|    N 10|
|:-------------------------|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push N + pop N (findmin)  |  10.926M|  11.385k| 25.261k| 75.173k|249.297k|733.982k|  2.317M|  4.429M|  6.602M|  7.987M|
|push N + pop N (bsearch)  | 128.560k| 449.969k|  1.231M|  2.281M|  3.118M|  3.650M|  4.211M|  4.892M|  5.867M|  7.084M|
|push N + pop N (rb_heap)  |   1.099M|   1.192M|  1.316M|  1.457M|  1.638M|  1.832M|  2.128M|  2.527M|  3.239M|  4.368M|
|push N + pop N (c++ stl)  |   6.313M|   6.762M|  7.137M|  7.631M|  8.173M|  8.520M|  9.300M|  9.775M| 10.309M| 11.391M|
|push N + pop N (c_dheap)  |   8.789M|   9.784M| 10.786M| 11.537M| 12.206M| 13.245M| 13.419M| 13.857M| 14.074M| 14.656M|