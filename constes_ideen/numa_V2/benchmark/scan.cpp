#include <vector>
#include <functional>
#include <benchmark/benchmark.h>
#include <omp.h>

#include "numa_extensions.hpp"
#include "numa_algorithm.hpp"
#include "allocator_adaptor.hpp"

using ValueType = float;
using Container = std::vector<ValueType, numa::no_init_allocator<ValueType>>;
static constexpr ValueType initVal = 0.01;


static void Args(benchmark::internal::Benchmark* b) {
  const auto lowerLimit = 15;
  const auto upperLimit = 32;

  for (auto x = lowerLimit; x <= upperLimit; ++x) {
        b->Args({int64_t{1} << x});

  }
}

static void numa_scan(benchmark::State& state) {
    const int block_size = (1 << 17);
    numa_extensions<Container> X(state.range(0));
    X.scan_init(block_size, initVal);

    for (auto _ : state) {
        numa_scan(X.begin(), X.end(), X.begin(), std::plus<>{}, ValueType{0}, block_size);

        benchmark::DoNotOptimize(X.data());
        benchmark::ClobberMemory();
    }
}




BENCHMARK(numa_scan)->Apply(Args)->UseRealTime();
BENCHMARK_MAIN();