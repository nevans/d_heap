# Iteration per second (i/s)

|                  |N 300_000|N 100_000|N 30_000|N 10_000| N 3_000| N 1_000|   N 300|   N 100|    N 30|    N 10|
|:-----------------|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push N (findmin)  |  11.798M|  11.927M| 12.073M| 12.095M| 11.980M| 12.017M| 11.842M| 11.694M| 11.814M| 11.371M|
|push N (bsearch)  |  64.778k| 225.934k|638.747k|  1.221M|  1.714M|  2.020M|  2.409M|  2.899M|  3.646M|  4.433M|
|push N (rb_heap)  |   4.652M|   4.665M|  4.635M|  4.643M|  4.660M|  4.684M|  4.798M|  4.927M|  5.275M|  5.756M|
|push N (c++ stl)  |   8.449M|   8.747M|  8.878M|  9.174M|  9.051M|  9.153M|  9.009M|  8.912M|  8.193M|  7.187M|
|push N (c_dheap)  |  12.430M|  12.444M| 12.496M| 12.526M| 12.493M| 12.519M| 12.625M| 12.527M| 12.406M| 12.252M|
