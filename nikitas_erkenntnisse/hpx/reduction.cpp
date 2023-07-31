#include <hpx/local/algorithm.hpp>
#include <hpx/local/future.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/iostream.hpp>
#include <hpx/include/partitioned_vector.hpp>

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
	HPX_REGISTER_PARTITIONED_VECTOR(ValueType);

	std::vector<hpx::id_type> locs = hpx::find_all_localities()
	std::size_t num_segments = locs.size();

	// one segment for each localitiy, if more round robin is used
	auto layout = hpx::container_layout(num_segments, locs);
	hpx::partitioned_vector<ValueType> X(state.range(0), layout);
	
	// perform uninitilized fill HPX style here
	hpx::uninitilized_fill(X.begin(), X.end(), ValueType{1});
	ValueType sum;

	for(auto _ : state)
	{
		sum = 0;
		// https://hpx-docs.stellar-group.org/latest/html/libs/core/algorithms/api/for_loop_reduction.html#_CPPv4I00EN3hpx12experimental9reductionEN3hpx8parallel6detail16reduction_helperI1TNSt7decay_tI2OpEEEER1TRK1TRR2Op
		benchmark::DoNotOptimize(hpx::experimental::reduction(&sum, X, +));
	}
}

int main(int argc, char* argv[])
{
	::benchmark::Initialize(&argc, argv);
	::benchmark::RunSpecifiedBenchmarks();

    return 0;
}

BENCHMARK(benchReduceHPX)->Apply(Args)->UseRealTime();