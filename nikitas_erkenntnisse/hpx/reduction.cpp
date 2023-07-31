#include <hpx/local/algorithm.hpp>
#include <hpx/local/future.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/iostream.hpp>

#include <benchmark/benchmark.h>

#include "allocator_adaptor.hpp"

using ValueType = float;
using ContainerType = std::vector<float, numa::no_init_allocator<float>>;

static void Args(benchmark::internal::Benchmark* b) {
  const int64_t lowerLimit = 3;
  const int64_t upperLimit = 10;

  for (auto x = lowerLimit; x <= upperLimit; ++x) {
        b->Args({int64_t{1} << x});
  }
}

void benchReduceHPX(benchmark::State& state)
{
	// Initilize container
	ContainerType X(state.range(0));

	// Figure out how many localities are used and which ones get how much data
	std::vector<hpx::id_type> localities = hpx::find_all_localities();
	size_t partSize = X.size() / localities.size();


	
	for(auto _ : state)
	{
		benchReduceHPX(state.range(0));
	}
}

int main(int argc, char* argv[])
{
	::benchmark::Initialize(&argc, argv);
	::benchmark::RunSpecifiedBenchmarks();

    return 0;
}

BENCHMARK(benchReduceHPX)->Apply(Args)->UseRealTime();