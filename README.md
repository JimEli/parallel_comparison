# Simplistic C++ Concurrent and Parallel API Comparison

Compares different sequential and concurrent programming methods to fill an array with sequential integral values. No optimizations applied.

Results on my Intel Core i3 5005U 2.00GHz w/Intel HD 5500 GPU:
  Number of processors: 4, number of iterations: 50
  sequential for loop    : 0.00516838
  std::generate          : 0.00847837
  ppl parallel_for       : 0.0130731
  ppl parallel_invoke    : 0.00465069
  c++17 parallel for_each: 0.00470779
  c++ AMP                : 0.0268972
  threads                : 0.00527882
  openMP                 : 0.0059868

Notes:
* Compiled/tested with MS Visual Studio 2017 Community (v141), and Windows SDK version 10.0.17134.0 (32 & 64-bit). Using the following options: /openmp /std:c++17, Conformance mode: No
* Change Log: 12/31/2018: Initial release. JME
