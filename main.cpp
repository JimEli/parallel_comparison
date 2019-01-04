/*************************************************************************
* Title: Concurrent/parallel processing comparison
* File: main.cpp
* Author: James Eli
* Date: 12/31/2018
*
* Compares different sequential and concurrent programming methods to fill
* an array with sequential integral values. No optimizations applied.
*
* Results on my Intel Core i3 5005U 2.00GHz w/Intel HD 5500 GPU:
*  Number of processors: 4, number of iterations: 50
*  sequential for loop    : 0.00553748
*  std::generate          : 0.00919595
*  ppl parallel_for       : 0.0136484
*  ppl parallel_invoke    : 0.00466764
*  c++17 parallel for_each: 0.004742
*  c++ AMP                : 0.0268908
*  threads                : 0.00539937
*  openMP                 : 0.00623423
*  tbb                    : 0.00572005
*
* Notes:
*  (1) Compile release x64 version for best results.
*  (2) Compiled/tested with MS Visual Studio 2017 Community (v141), and
*      Windows SDK version 10.0.17134.0 (32 & 64-bit). Using the following
*      options: /openmp /std:c++17, Conformance mode: No
*************************************************************************
* Change Log:
*   12/31/2018: Initial release. JME
*   1/4/2018: Added TBB version. JME
*************************************************************************/

#include <iostream>  
#include <memory>    
#include <exception>
#include <algorithm> 
#include <numeric>
#include <ppl.h>
#include <execution>
#include <amp.h>
#include <thread>
#include <vector>
#include <omp.h>
#include "tbb\parallel_for.h"
#include "tbb\task_scheduler_init.h"

constexpr unsigned MAX_TESTS{ 9 };
constexpr unsigned NUM_ITERATIONS{ 50 };
constexpr unsigned ARRAY_SIZE{ 10'000'000 };
constexpr unsigned NUM_THREADS{ 4 };

// Thread slice for array filling.
void tFill(const unsigned from, const unsigned size, unsigned* arr)
{
	for (unsigned i = from; i < from + size; i++)
		arr[i] = i;
}

// Sequential for loop.
void seq(unsigned* arr, unsigned size)
{
	for (unsigned i = 0; i < size; i++)
		arr[i] = i;
}

// Sequential std::generate.
void gen(unsigned* arr, unsigned size)
{
	std::generate(arr, arr + size, [i = 0]() mutable { return i++; });
}

// PPL parallel_for loop.
void ppf(unsigned* arr, unsigned size)
{
	Concurrency::parallel_for<std::size_t>(std::size_t(0), std::size_t(size), [&arr](unsigned i) { arr[i] = i; }, concurrency::static_partitioner());
}

// PPL parallel_invoke with 4 threads.
void ppi(unsigned* arr, unsigned size)
{
	Concurrency::parallel_invoke(
		[&] { tFill(0u, size / NUM_THREADS, arr); },
		[&] { tFill(size / NUM_THREADS, size / NUM_THREADS, arr); },
		[&] { tFill(2 * size / NUM_THREADS, size / NUM_THREADS, arr); },
		[&] { tFill(3 * size / NUM_THREADS, size / NUM_THREADS, arr); }
	);
}

// C++17 parallel for_each loop.
void cpp(unsigned* arr, unsigned size)
{
	std::for_each(std::execution::par_unseq, arr, arr + size, [arr](auto& a) { a = &a - &arr[0]; });
}

// C++ AMP version.
void amp(unsigned* arr, unsigned size)
{
	using namespace concurrency;

	accelerator default_device;

	if (default_device != accelerator(accelerator::direct3d_ref))
	{
		array<unsigned, 1> av(size, arr);

		parallel_for_each(av.extent, [&av](concurrency::index<1> i) restrict(amp) { av[i] = i[0]; });
		// Synchronise?
		//av.accelerator_view.wait();
		concurrency::copy(av, arr);
	}
}

// Concurrent thread version.
void thd(unsigned* arr, unsigned size)
{
	std::vector<std::thread> threads;
	unsigned numThreads = omp_get_num_procs();

	// Spawns threads. 
	for (unsigned i = 0; i < numThreads; i++)
		threads.push_back(std::thread(tFill, (i * size) / numThreads, size / numThreads, std::ref(arr)));
	for (auto& th : threads)
		th.join();
}

// TBB parallel_for loop.
void itb(unsigned* arr, unsigned size)
{
	tbb::task_scheduler_init init;       // Automatic number of threads
	//tbb::task_scheduler_init init(4);  // Explicit number of threads
	tbb::parallel_for(tbb::blocked_range<unsigned>(0, size),
		[=](const tbb::blocked_range<unsigned>& r) {
			for (unsigned i = r.begin(); i != r.end(); ++i)
				arr[i] = i;
		}, 
		tbb::auto_partitioner()
	);
}

// OMP version.
void opn(unsigned* arr, unsigned size)
{
#pragma omp parallel for
	for (int i = 0; i < static_cast<int>(size); i++)
		arr[i] = i;
}

// Array filling function names.
std::string fillMethodDescription[] =
{
	"sequential for loop    ",
	"std::generate          ",
	"ppl parallel_for       ",
	"ppl parallel_invoke    ",
	"c++17 parallel for_each",
	"c++ AMP                ",
	"threads                ",
	"openMP                 ",
	"tbb                    "
};

// Array of function pointers.
void(*pFill[])(unsigned*, unsigned) = { seq, gen, ppf, ppi, cpp, amp, thd, opn, itb };

int main()
{
	// Report number of processors.
	std::cout << "Number of processors: " << omp_get_num_procs() << ", number of iterations: " << NUM_ITERATIONS << std::endl;
	
	// Benchmark loop.
	for (int numTests = 0; numTests < MAX_TESTS; numTests++)
	{
		// Accumulative running time.
		double t = 0.;

		for (int i=0; i<NUM_ITERATIONS; i++)
		{
			try
			{
				std::unique_ptr<unsigned[]> a = std::make_unique<unsigned[]>(ARRAY_SIZE);
				
				// Start timer.
				double start = omp_get_wtime();

				pFill[numTests](a.get(), ARRAY_SIZE);

				// End timer.
				t += (omp_get_wtime() - start);

				// Assert correct results.
				if (!std::is_sorted(a.get(), a.get() + ARRAY_SIZE) || a[0] != 0 || a[ARRAY_SIZE - 1] != ARRAY_SIZE - 1)
				{
					std::cout << fillMethodDescription[numTests] << " failed!\n";
					exit(-1);
				}
			} 
			catch (const std::exception& ex)
			{
				std::cout << "Exception failure: " << ex.what() << std::endl;
				exit(-1);
			}
		}

		std::cout << fillMethodDescription[numTests] << ": " << t / NUM_ITERATIONS << std::endl;
	}

	return 0;
}
