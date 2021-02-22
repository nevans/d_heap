# Iteration per second (i/s)

|                          |N 300_000|N 100_000|N 30_000|N 10_000| N 3_000| N 1_000|   N 300|   N 100|    N 30|    N 10|
|:-------------------------|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push N + pop N (findmin)  |  10.846M|  11.383k| 25.080k| 75.084k|249.720k|736.712k|  2.312M|  4.399M|  6.595M|  8.257M|
|push N + pop N (bsearch)  | 128.948k| 447.811k|  1.232M|  2.265M|  3.112M|  3.642M|  4.228M|  4.890M|  5.855M|  7.060M|
|push N + pop N (rb_heap)  |   1.097M|   1.193M|  1.318M|  1.460M|  1.640M|  1.835M|  2.133M|  2.518M|  3.250M|  4.353M|
|push N + pop N (c++ stl)  |   6.258M|   6.619M|  7.125M|  7.594M|  8.059M|  8.711M|  9.245M|  9.702M| 10.477M| 11.366M|
|push N + pop N (c_dheap)  |   7.409M|   8.376M|  9.204M|  9.854M| 10.526M| 11.380M| 11.795M| 12.456M| 13.164M| 13.709M|