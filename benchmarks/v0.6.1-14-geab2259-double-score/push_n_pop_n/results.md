# Iteration per second (i/s)

|                          |N 300_000|N 100_000|N 30_000|N 10_000| N 3_000| N 1_000|   N 300|   N 100|    N 30|    N 10|
|:-------------------------|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push N + pop N (findmin)  |  10.912M|  11.385k| 25.272k| 75.167k|250.324k|738.177k|  2.318M|  4.451M|  6.623M|  8.256M|
|push N + pop N (bsearch)  | 128.786k| 449.278k|  1.231M|  2.225M|  3.080M|  3.613M|  4.223M|  4.872M|  5.979M|  6.942M|
|push N + pop N (rb_heap)  |   1.100M|   1.194M|  1.321M|  1.450M|  1.633M|  1.837M|  2.131M|  2.514M|  3.227M|  4.359M|
|push N + pop N (c++ stl)  |   6.316M|   6.718M|  7.181M|  7.612M|  8.194M|  8.659M|  9.253M|  9.766M| 10.337M| 11.286M|
|push N + pop N (c_dheap)  |   6.927M|   7.581M|  8.246M|  8.964M|  9.620M| 10.281M| 11.049M| 12.030M| 13.272M| 14.272M|
