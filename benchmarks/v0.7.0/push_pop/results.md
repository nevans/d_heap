# Iteration per second (i/s)

|                      |N 10_000_000|N 3_162_278|N 1_000_000|N 316_228|N 100_000|N 31_623|N 10_000|  N 3162|  N 1000|   N 316|   N 100|    N 32|    N 10|
|:---------------------|-----------:|----------:|----------:|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push + pop (amindel)  |      21.847|     67.578|    213.679|  629.966|   1.955k|  6.261k| 18.680k| 58.827k|210.029k|648.549k|  1.489M|  2.500M|  2.950M|
|push + pop (hmindel)  |     265.677|    257.987|    260.991|  817.379|   2.413k|  6.994k| 20.951k| 52.265k|174.192k|451.337k|  1.193M|  1.809M|  2.260M|
|push + pop (bsearch)  |     277.890|     1.002k|     5.189k|  27.428k|  96.148k|354.945k|779.477k|  1.469M|  1.892M|  2.110M|  2.679M|  3.108M|  3.928M|
|push + pop (rb_heap)  |    435.952k|   446.471k|   447.431k| 411.897k| 465.383k|490.704k|517.807k|577.083k|686.936k|773.094k|991.538k|  1.181M|  1.683M|
|push + pop (c++ stl)  |      1.763M|     2.237M|     3.060M|   4.261M|   5.072M|  5.984M|  6.101M|  6.544M|  6.888M|  7.304M|  7.660M|  7.986M|  8.483M|
|push + pop (c_dheap)  |      1.960M|     2.506M|     3.453M|   5.317M|   5.604M|  6.728M|  6.807M|  7.182M|  8.782M|  8.963M|  7.952M|  9.370M| 10.805M|
|push + pop (heapmap)  |    383.707k|   423.581k|   571.600k| 832.856k|   1.276M|  1.820M|  2.191M|  2.288M|  2.622M|  2.684M|  3.038M|  3.253M|  3.969M|