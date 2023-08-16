#include <benchmark/benchmark.h>
#include <xsimd/xsimd.hpp>
#include <vector>
#include <iostream>

#include <oneapi/tbb/parallel_reduce.h>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/partitioner.h>

#include "arena.hpp"
#include "numa_extensions.hpp"

using ValueType = float;
using ContainerType = std::vector<ValueType, numa::no_init_allocator<ValueType>>;
using namespace oneapi;
using Partitioner = oneapi::tbb::simple_partitioner;
static constexpr size_t gs_divisor = 256;


static void Args(benchmark::internal::Benchmark* b) {
  const int64_t lowerLimit = 15;
  const int64_t upperLimit = 34;

  for (auto x = lowerLimit; x <= upperLimit; ++x) {
        b->Args({int64_t{1} << x});
  }
}

static void DefaultOmp(benchmark::State& state) {
    std::vector<ValueType> X(state.range(0), 1);

    ValueType sum;
    for (auto _ : state) {
        sum = 0;
        #pragma omp parallel for simd reduction(+ : sum)
        for (size_t i = 0; i < X.size(); i++) {
            sum += X[i];
        }
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void DefaultTbb(benchmark::State& state) {
    std::vector<ValueType> X(state.range(0), 1);

    ValueType sum;
    for(auto _ : state) {
        sum = tbb::parallel_reduce(tbb::blocked_range<size_t>(0, X.size()), ValueType{0}, [&] (const tbb::blocked_range<size_t>& r, ValueType ret) -> ValueType {
                for (auto j = r.begin(); j < r.end(); j++) {
                    ret += X[j];
                }
                return ret;
        }, std::plus<>{});
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void Omp(benchmark::State& state){
    int numa_nodes = omp_get_num_places();
    numa_extensions<ContainerType> X(numa_nodes, state.range(0), 1);

    ValueType sum;
    for (auto _ : state){
        sum = 0;
        #pragma omp parallel for simd reduction(+ : sum)
        for(size_t i = 0; i < X.size(); i++){
            sum += X[i];
        }
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void Omp2(benchmark::State& state){
    int numa_nodes = omp_get_num_places();
    numa_extensions<ContainerType> X(state.range(0), 1);

    ValueType sum;
    for (auto _ : state){
        sum = 0;
        #pragma omp parallel for simd reduction(+ : sum)
        for (size_t i = 0; i < X.size(); i++){
            sum += X[i];
        }
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void OmpSIMD(benchmark::State& state){
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>;
    int numa_nodes = omp_get_num_places();
    numa_extensions<ContainerTypeAligned> X(numa_nodes, state.range(0), 1);

    using b_type = xsimd::batch<ValueType>;
    int inc = b_type::size;

    ValueType sum;
    for (auto _ : state){
        sum = 0;
        #pragma omp parallel for reduction(+ : sum) 
        for(size_t i = 0; i < X.size(); i+=inc){
            b_type x_vec = b_type::load_aligned(&X[i]);
            sum += xsimd::reduce_add(x_vec);
        }
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void OmpSIMD2(benchmark::State& state){
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>;
    int numa_nodes = omp_get_num_places();
    numa_extensions<ContainerTypeAligned> X(state.range(0), 1);

    using b_type = xsimd::batch<ValueType>;
    int inc = b_type::size;

    ValueType sum;
    for (auto _ : state){
        sum = 0;
        #pragma omp parallel for reduction(+ : sum) 
        for(size_t i = 0; i < X.size(); i+=inc){
            b_type x_vec = b_type::load_aligned(&X[i]);
            sum += xsimd::reduce_add(x_vec);
        }
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void OmpNesting(benchmark::State& state){
    omp_set_dynamic(false);
    omp_set_max_active_levels(2);                                                                   
    int numa_nodes = omp_get_num_places();
    int thrds_per_node = omp_get_num_procs() / numa_nodes;
    numa_extensions<ContainerType> X(numa_nodes, state.range(0), 1);

    ValueType sum;
    for (auto _ : state){
        sum = 0;
        #pragma omp parallel num_threads(numa_nodes) proc_bind(spread)
        {
            const auto range = X.get_range(omp_get_place_num());
            #pragma omp parallel for simd num_threads(thrds_per_node) proc_bind(master) reduction(+ : sum)
            for(size_t i = range.first; i < range.second; i++){
                sum += X[i];
            }
        }
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void OmpNesting2(benchmark::State& state){
    omp_set_dynamic(false);
    omp_set_max_active_levels(2);
    int numa_nodes = omp_get_num_places();
    int thrds_per_node = omp_get_num_procs() / numa_nodes;
    numa_extensions<ContainerType> X(numa_nodes, thrds_per_node, state.range(0), 1);

    ValueType sum;
    for (auto _ : state){
        sum = 0;
        #pragma omp parallel num_threads(numa_nodes) proc_bind(spread)
        {
            const auto range = X.get_range(omp_get_place_num());
            #pragma omp parallel for simd num_threads(thrds_per_node) proc_bind(master) reduction(+ : sum)
            for(size_t i = range.first; i < range.second; i++){
                sum += X[i];
            }
        }
        benchmark::DoNotOptimize(&sum);
        benchmark::ClobberMemory();
    }
}

static void Tbb(benchmark::State& state){
    numa::ArenaMgtTBB arenas;                    
    numa_extensions<ContainerType> X(arenas.get_nodes(), state.range(0), 1);
    std::vector<ValueType> node_sums(arenas.get_nodes());
    
    int gs = (state.range(0) / arenas.get_nodes()) / gs_divisor;
    ValueType result;
    Partitioner part;


    for (auto _ : state){
        result = 0;
        arenas.execute([&] (const int i) {
            auto range = X.get_range(i);
            node_sums[i] = tbb::parallel_reduce(tbb::blocked_range<size_t>(range.first, range.second, gs), ValueType{0}, 
                                            [&] (const tbb::blocked_range<size_t> r, ValueType ret) -> ValueType {
                #pragma omp simd reduction(+ : ret)
                for (auto j = r.begin(); j < r.end(); j++) {
                    ret += X[j];
                }
                return ret;
            }, std::plus<>{}, part);
        });

        for (auto sum : node_sums) result += sum;
        benchmark::DoNotOptimize(&result);
        benchmark::ClobberMemory();
    }
}

static void Tbb2(benchmark::State& state){
    numa::ArenaMgtTBB arenas;                    
    numa_extensions<ContainerType> X(arenas, state.range(0), 1);
    std::vector<ValueType> node_sums(arenas.get_nodes());
    
    int gs = (state.range(0) / arenas.get_nodes()) / gs_divisor;
    ValueType result;
    Partitioner part;

    for (auto _ : state){
        result = 0;
        arenas.execute([&] (const int i) {
            auto range = X.get_range(i);
            node_sums[i] = tbb::parallel_reduce(tbb::blocked_range<size_t>(range.first, range.second, gs), ValueType{0}, 
                                            [&] (const tbb::blocked_range<size_t> r, ValueType ret) -> ValueType {
                #pragma omp simd reduction(+ : ret)
                for (auto j = r.begin(); j < r.end(); j++) {
                    ret += X[j];
                }
                return ret;
            }, std::plus<>{}, part);
        });

        for (auto sum : node_sums) result += sum;    
        benchmark::DoNotOptimize(&result);
        benchmark::ClobberMemory();
    }
}

static void TbbSIMD(benchmark::State& state){
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>;
    numa::ArenaMgtTBB arenas;                    
    numa_extensions<ContainerTypeAligned> X(arenas.get_nodes(), state.range(0), 1);
    std::vector<ValueType> node_sums(arenas.get_nodes());
    
    using b_type = xsimd::batch<ValueType>;
    size_t inc = b_type::size;

    ValueType result;
    tbb::auto_partitioner part;


    for (auto _ : state){
        result = 0;
        arenas.execute([&] (const int i) {
            auto range = X.get_range(i);
            size_t end = range.first + (((range.second  - range.first) + 1) / inc);
            node_sums[i] = tbb::parallel_reduce(tbb::blocked_range<size_t>(range.first, end), ValueType{0}, 
                                            [&] (const tbb::blocked_range<size_t> r, ValueType ret) -> ValueType {
                for (auto j = r.begin(); j < r.end(); j++) {
                    size_t index = range.first + (j - range.first) * inc;

                    b_type x_vec = b_type::load_aligned(&X[index]);
                    ret += xsimd::reduce_add(x_vec); 
                }
                return ret;
            }, std::plus<>{}, part);
        });

        for (auto sum : node_sums) result += sum;
        benchmark::DoNotOptimize(&result);
        benchmark::ClobberMemory();
    }
}

static void TbbSIMD2(benchmark::State& state){
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>;
    numa::ArenaMgtTBB arenas;                    
    numa_extensions<ContainerTypeAligned> X(arenas, state.range(0), 1);
    std::vector<ValueType> node_sums(arenas.get_nodes());
    
    using b_type = xsimd::batch<ValueType>;
    size_t inc = b_type::size;

    ValueType result;
    tbb::auto_partitioner part;

    for (auto _ : state){
        result = 0;
        arenas.execute([&] (const int i) {
            auto range = X.get_range(i);
            size_t end = range.first + (((range.second  - range.first) + 1) / inc);
            node_sums[i] = tbb::parallel_reduce(tbb::blocked_range<size_t>(range.first, end), ValueType{0}, 
                                            [&] (const tbb::blocked_range<size_t> r, ValueType ret) -> ValueType {
                for (auto j = r.begin(); j < r.end(); j++) {
                    size_t index = range.first + (j - range.first) * inc;

                    b_type x_vec = b_type::load_aligned(&X[index]);
                    ret += xsimd::reduce_add(x_vec); 
                }
                return ret;
            }, std::plus<>{}, part);
        });

        for (auto sum : node_sums) result += sum;
        benchmark::DoNotOptimize(&result);
        benchmark::ClobberMemory();
    }
}


BENCHMARK(DefaultOmp)->Apply(Args)->UseRealTime();
BENCHMARK(DefaultTbb)->Apply(Args)->UseRealTime();
BENCHMARK(Omp)->Apply(Args)->UseRealTime();
BENCHMARK(Omp2)->Apply(Args)->UseRealTime();
BENCHMARK(OmpSIMD)->Apply(Args)->UseRealTime();
BENCHMARK(OmpSIMD2)->Apply(Args)->UseRealTime();
BENCHMARK(OmpNesting)->Apply(Args)->UseRealTime();
BENCHMARK(OmpNesting2)->Apply(Args)->UseRealTime();
BENCHMARK(Tbb)->Apply(Args)->UseRealTime();
BENCHMARK(Tbb2)->Apply(Args)->UseRealTime();
BENCHMARK(TbbSIMD)->Apply(Args)->UseRealTime();
BENCHMARK(TbbSIMD2)->Apply(Args)->UseRealTime();
BENCHMARK_MAIN();