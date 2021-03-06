# Simple C++ Concurrent and Parallel API Comparison

This program compares several different sequential and concurrent programming methods used to fill an array with sequential integral values. Utilizes threads, C++ PPL, C++ AMP, C++17 Parallel, TBB and OpenMP libraries. Utilizes a simple timing benchmark. No specific optimizations applied. 

Compares the following algorithms:

### Sequential Algorithms
Basic *for* loop:
```C++
for (unsigned i = 0; i < size; i++)
    arr[i] = i;
```

C++ Standard Template Library *std::generate* function:
```C++
  std::generate(arr, arr + size, [i = 0]() mutable { return i++; });
```

### Parallel Algorithms
Microsoft Parallel Patterns Library *parallel_for* loop:
```C++
  Concurrency::parallel_for<std::size_t>(std::size_t(0), std::size_t(size), [&arr](unsigned i) { arr[i] = i; }, concurrency::static_partitioner());
```

PPL *parallel_invoke* function with 4 threads:
```C++
Concurrency::parallel_invoke(
    [&] { tFill(0u, size / NUM_THREADS, arr); },
    [&] { tFill(size / NUM_THREADS, size / NUM_THREADS, arr); },
    [&] { tFill(2 * size / NUM_THREADS, size / NUM_THREADS, arr); },
    [&] { tFill(3 * size / NUM_THREADS, size / NUM_THREADS, arr); }
  );
```

C++17 parallel extensions *for_each* loop:
```C++
  std::for_each(std::execution::par_unseq, arr, arr + size, [arr](auto& a) { a = &a - &arr[0]; });
```

C++ AMP version:
```C++
  array<unsigned, 1> av(size, arr);
  parallel_for_each(av.extent, [&av](concurrency::index<1> i) restrict(amp) { av[i] = i[0]; });
  concurrency::copy(av, arr);
}
```

Concurrent thread version:
```C++
void tFill(const unsigned from, const unsigned size, unsigned* arr) {
  for (unsigned i = from; i < from + size; i++) arr[i] = i;
}

  for (unsigned i = 0; i < numThreads; i++)
    threads.push_back(std::thread(tFill, (i * size) / numThreads, size / numThreads, std::ref(arr)));
  for (auto& th : threads)
    th.join();
```

OpenMP version.
```C++
#pragma omp parallel for
  for (int i = 0; i < static_cast<int>(size); i++)
    arr[i] = i;
```

Intel Thread Building Blocks (TBB) version.
```C++
	tbb::task_scheduler_init init;  // Automatic number of threads
	tbb::parallel_for(tbb::blocked_range<unsigned>(0, size),
		[=](const tbb::blocked_range<unsigned>& r) {
			for (unsigned i=r.begin(); i!=r.end(); ++i)
				arr[i] = i;
		}, tbb::auto_partitioner()
    );
```

### Smaple Results
Captured on my Intel Core i3 5005U 2.00GHz w/Intel HD 5500 GPU (2 core/4 thread laptop computer):
```text
  Number of processors: 4, number of iterations: 50
  sequential for loop    : 0.00553748
  std::generate          : 0.00919595
  ppl parallel_for       : 0.0136484
  ppl parallel_invoke    : 0.00466764
  c++17 parallel for_each: 0.004742
  c++ AMP                : 0.0268908
  threads                : 0.00539937
  openMP                 : 0.00623423
  tbb                    : 0.00572005
```

Notes:
* Compiled/tested with MS Visual Studio 2017 Community (v141), and Windows SDK version 10.0.17134.0 (32 & 64-bit). Using the following options: /openmp /std:c++17, Conformance mode: No
* Change Log: 12/31/2018: Initial release. JME
