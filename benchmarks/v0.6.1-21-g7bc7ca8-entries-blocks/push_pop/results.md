# Iteration per second (i/s)

|                      |N 10_000_000|N 3_162_278|N 1_000_000|N 316_228|N 100_000|N 31_623|N 10_000|  N 3162|  N 1000|   N 316|   N 100|    N 32|    N 10|
|:---------------------|-----------:|----------:|----------:|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push + pop (amindel)  |      23.270|     75.086|    241.490|  660.047|   2.048k|  7.061k| 19.796k| 65.634k|189.569k|646.889k|  1.519M|  2.537M|  3.250M|
|push + pop (hmindel)  |     301.384|    300.496|    301.361|  938.636|   2.791k|  7.790k| 21.872k| 55.273k|180.430k|451.915k|  1.166M|  1.788M|  2.205M|
|push + pop (bsearch)  |     305.239|     1.068k|     5.889k|  29.411k|  96.648k|374.223k|793.246k|  1.472M|  1.866M|  2.106M|  2.761M|  3.158M|  4.140M|
|push + pop (rb_heap)  |    445.050k|   462.266k|   468.821k| 454.281k| 473.908k|499.662k|526.380k|595.628k|696.962k|788.806k|  1.006M|  1.184M|  1.704M|
|push + pop (c++ stl)  |      1.858M|     2.416M|     3.332M|   4.508M|   5.162M|  6.224M|  6.276M|  6.620M|  7.114M|  7.430M|  7.835M|  7.990M|  8.593M|
|push + pop (c_dheap)  |      1.236M|     1.712M|     2.474M|   3.705M|   3.956M|  4.331M|  4.731M|  5.409M|  5.913M|  6.690M|  6.667M|  7.917M|  9.569M|
|push + pop (heapmap)  |    305.953k|   373.288k|   491.296k| 792.132k|   1.205M|  1.384M|  1.584M|  1.713M|  1.915M|  2.143M|  2.168M|  2.632M|  3.115M|
