# Iteration per second (i/s)

|                      |N 10_000_000|N 3_162_278|N 1_000_000|N 316_228|N 100_000|N 31_623|N 10_000|  N 3162|  N 1000|   N 316|   N 100|    N 32|    N 10|
|:---------------------|-----------:|----------:|----------:|--------:|--------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|-------:|
|push + pop (amindel)  |      23.764|     72.333|    221.253|  665.370|   2.262k|  6.760k| 19.049k| 67.586k|210.009k|648.709k|  1.489M|  2.584M|  3.271M|
|push + pop (hmindel)  |     301.577|    302.642|    302.501|  943.267|   2.800k|  7.775k| 21.957k| 55.550k|180.205k|448.039k|  1.178M|  1.795M|  2.246M|
|push + pop (bsearch)  |     303.158|     1.077k|     5.688k|  29.692k|  94.852k|364.203k|766.295k|  1.498M|  1.862M|  2.138M|  2.612M|  3.216M|  4.018M|
|push + pop (rb_heap)  |    447.550k|   465.693k|   471.464k| 450.819k| 472.237k|489.786k|527.295k|590.291k|691.840k|781.673k|  1.002M|  1.187M|  1.695M|
|push + pop (c++ stl)  |      1.857M|     2.410M|     3.324M|   4.496M|   5.182M|  6.098M|  6.191M|  6.547M|  7.168M|  7.318M|  7.871M|  8.057M|  8.589M|
|push + pop (c_dheap)  |      2.029M|     2.643M|     3.717M|   5.344M|   6.021M|  6.766M|  7.434M|  8.290M|  8.917M|  9.760M|  9.851M| 10.787M| 11.680M|
|push + pop (heapmap)  |    328.215k|   391.579k|   531.581k| 835.901k|   1.296M|  1.469M|  1.715M|  1.887M|  2.110M|  2.420M|  2.402M|  2.855M|  3.360M|
