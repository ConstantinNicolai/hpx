#include <benchmark/benchmark.h>
#include <vector>

#include "allocator_adaptor.hpp"
#include "numa_adaptor.hpp"
#include "arenaV3.hpp"

#include <hpx/hpx.hpp>
#include <hpx/include/parallel_transform.hpp>
#include <hpx/include/memory.hpp>

using ValueType = float;
using ContainerType = std::vector<ValueType, numa::no_init_allocator<ValueType>>;
using namespace oneapi;
using Partitioner = tbb::static_partitioner;
static constexpr int numa_nodes = 4;
static constexpr int thrds_per_node = 32;

static void Args(benchmark::internal::Benchmark* b) {
  const int64_t lowerLimit = 15;
  const int64_t upperLimit = 33;

  for (auto x = lowerLimit; x <= upperLimit; ++x) {
    b->Args({int64_t{1} << x});
  }
}

void setCustomCounter(benchmark::State& state, std::string name) {
  state.counters["Elements"] = state.range(0);
  state.counters["Bytes"] = 3 * state.range(0) * sizeof(ValueType);
  state.SetLabel(name);
}

static void benchTransformHPX(benchmark::State& state) {
    constexpr int numLocalities = 4; // Assuming 4 NUMA localities for illustration
    constexpr int dataSize = state.range(0);

    std::vector<std::vector<ValueType, hpx::memory::locality_allocator<ValueType>>> X(
        numLocalities, std::vector<ValueType, hpx::memory::locality_allocator<ValueType>>(
                          dataSize, 1, hpx::memory::locality_allocator<ValueType>{}));

    std::vector<std::vector<ValueType, hpx::memory::locality_allocator<ValueType>>> Y(
        numLocalities, std::vector<ValueType, hpx::memory::locality_allocator<ValueType>>(
                          dataSize, 1, hpx::memory::locality_allocator<ValueType>{}));

    constexpr ValueType alpha = 2;

    for (auto _ : state) {
        for (int i = 0; i < numLocalities; ++i) {
            hpx::transform(hpx::execution::par(hpx::execution::task),
                           X[i].begin(), X[i].end(), Y[i].begin(),
                           [alpha](ValueType x) { return alpha * x; });
        }
    }
}

int main(int argc, char** argv) {
  // Perform any necessary initialization here

  // Run benchmarks
  benchmark::Initialize(&argc, argv);

  BENCHMARK(Args)->UseRealTime();
  BENCHMARK(benchTransformHPX)->Apply(Args)->UseRealTime();

  benchmark::RunSpecifiedBenchmarks();

  // Perform any necessary cleanup here

  return 0;
}
