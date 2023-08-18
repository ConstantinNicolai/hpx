#include <hpx/local/algorithm.hpp>
#include <hpx/local/future.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/iostream.hpp>
#include <hpx/include/partitioned_vector.hpp>
#include <hpx/runtime_distributed/find_localities.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/memory.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/config.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/hpx.hpp>
#include <hpx/barrier.hpp>

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static void Args(benchmark::internal::Benchmark* b) {
		const int64_t lowerLimit = 15;
		const int64_t upperLimit = 33;

		for (auto x = lowerLimit; x <= upperLimit; ++x) {
				b->Args({int64_t{1} << x});
		}
}

using ValueType = float;

int hpx_main(int argc, char* argv[])
{
	/*
	for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
	*/

	std::cout << "Locality " << hpx::get_locality_id() << " is completly done" << std::endl;
    return hpx::finalize();
}

void benchReduceHPXTesting(benchmark::State& state, int argc, char* argv[])
{
	std::cout << state.range(0) << std::endl;

	printf("You have entered %d arguments:\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    // run hpx_main on all localities
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.cfg = cfg;

	for(auto _ : state)
	{
    	//hpx::init(argc, argv, init_args);
	}
}

int main(int argc, char* argv[])
{
	::benchmark::Initialize(&argc, argv);
	benchmark::RegisterBenchmark("test", benchReduceHPXTesting, argc, argv)->Apply(Args)->UseRealTime();
	//BENCHMARK(benchReduceHPXTesting)->Apply(Args)->UseRealTime();
	::benchmark::RunSpecifiedBenchmarks();
}

