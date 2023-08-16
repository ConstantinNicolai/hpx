#include <benchmark/benchmark.h>
#include <vector>
#include <xsimd/xsimd.hpp>
#include <iostream>

#include "allocator_adaptor.hpp"
#include "numa_extensions.hpp"
#include "arena.hpp"

using ValueType = float;
using ContainerType = std::vector<ValueType, numa::no_init_allocator<ValueType>>;

using namespace oneapi;
using Partitioner = tbb::static_partitioner;

static void Args(benchmark::internal::Benchmark* b) {
  const int64_t lowerLimit = 33;
  const int64_t upperLimit = 33;

  for (auto x = lowerLimit; x <= upperLimit; ++x) {
    b->Args({int64_t{1} << x});
  }
}

static void DefaultOmp(benchmark::State& state) {
    std::vector<ValueType> X(state.range(0), 1);
    std::vector<ValueType> Y(state.range(0), 2);

    constexpr ValueType alpha = 2;

    for (auto _ : state) {
        #pragma omp parallel for
        for (size_t i = 0; i < X.size(); i++) {
            Y[i] = alpha * X[i] + Y[i];
        }
        benchmark::ClobberMemory();
    }
}

static void DefaultTbb(benchmark::State& state) {
    std::vector<ValueType> X(state.range(0), 1);
    std::vector<ValueType> Y(state.range(0), 2);

    constexpr ValueType alpha = 2;

    for (auto _ : state) {
        tbb::parallel_for(tbb::blocked_range<size_t>(0, X.size()), [&] (const tbb::blocked_range<size_t> r) {
            #pragma omp simd
            for (auto i = r.begin(); i < r.end(); i++) {
                Y[i] = alpha * X[i] + Y[i];
            }
        });
        benchmark::ClobberMemory();
    }
}

static void Omp(benchmark::State& state){
    const int numa_nodes = omp_get_num_places();
    numa_extensions<ContainerType> X(numa_nodes, state.range(0), 1);
    numa_extensions<ContainerType> Y(numa_nodes, state.range(0), 1); 
    constexpr ValueType alpha = 2;

    for (auto _ : state){
        #pragma omp parallel for 
        for (size_t i = 0; i < X.size(); i++){
            Y[i] = alpha * X[i] + Y[i];
        }
        benchmark::ClobberMemory();
    }
}

static void Omp2(benchmark::State& state){
    numa_extensions<ContainerType> X(state.range(0), 1);
    numa_extensions<ContainerType> Y(state.range(0), 1);
    const int numa_nodes = omp_get_num_places();

    constexpr ValueType alpha = 2;

    for (auto _ : state){
        #pragma omp parallel for 
        for (size_t i = 0; i < X.size(); i++){
            Y[i] = alpha * X[i] + Y[i];
        }
        benchmark::ClobberMemory();
    }
}

static void OmpSIMD(benchmark::State& state){
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>;
    const int numa_nodes = omp_get_num_places();
    numa_extensions<ContainerTypeAligned> X(numa_nodes, state.range(0), 1);
    numa_extensions<ContainerTypeAligned> Y(numa_nodes, state.range(0), 1);
    constexpr ValueType alpha = 2;

    using b_type = xsimd::batch<ValueType>;
    size_t inc = b_type::size;

    for (auto _ : state){
        #pragma omp parallel for 
        for (size_t i = 0; i < X.size(); i+=inc){
            b_type x_vec = b_type::load_aligned(&X[i]);
            b_type y_vec = b_type::load_aligned(&Y[i]);
            b_type a_vec = b_type(alpha);

            b_type r_vec = xsimd::fma(a_vec, x_vec, y_vec);
            r_vec.store_aligned(&Y[i]);
        }
        benchmark::ClobberMemory();
    }
}

static void OmpSIMD2(benchmark::State& state){
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>;
    const int numa_nodes = omp_get_num_places();
    numa_extensions<ContainerTypeAligned> X(state.range(0), 1);
    numa_extensions<ContainerTypeAligned> Y(state.range(0), 1);
    constexpr ValueType alpha = 2;

    using b_type = xsimd::batch<ValueType>;
    size_t inc = b_type::size;

    for (auto _ : state){
        #pragma omp parallel for 
        for (size_t i = 0; i < X.size(); i+=inc){
            b_type x_vec = b_type::load_aligned(&X[i]);
            b_type y_vec = b_type::load_aligned(&Y[i]);
            b_type a_vec = b_type(alpha);

            b_type r_vec = xsimd::fma(a_vec, x_vec, y_vec);
            r_vec.store_aligned(&Y[i]);
        }
        benchmark::ClobberMemory();
    }

}

static void OmpNesting(benchmark::State& state){
    omp_set_dynamic(false);
    omp_set_max_active_levels(2);
    const int numa_nodes = omp_get_num_places();
    const int thrds_per_node = omp_get_num_procs() / numa_nodes;
    numa_extensions<ContainerType> X(numa_nodes, state.range(0), 1);
    numa_extensions<ContainerType> Y(numa_nodes, state.range(0), 1);
    
    constexpr ValueType alpha = 2;

    for (auto _ : state){
        #pragma omp parallel proc_bind(spread) num_threads(numa_nodes)
        {   
            const auto range = X.get_range(omp_get_place_num());
            #pragma omp parallel for proc_bind(master) num_threads(thrds_per_node) 
            for (size_t i = range.first; i < range.second; i++){
                Y[i] = alpha * X[i] + Y[i];
            }
        }
        benchmark::ClobberMemory();
    }
}

static void OmpNesting2(benchmark::State& state){
    const int numa_nodes = omp_get_num_places();
    const int thrds_per_node = omp_get_num_procs() / numa_nodes;
    numa_extensions<ContainerType> X(numa_nodes, thrds_per_node, state.range(0), 1);
    numa_extensions<ContainerType> Y(numa_nodes, thrds_per_node, state.range(0), 1);
    constexpr ValueType alpha = 2;

    for (auto _ : state){
        #pragma omp parallel proc_bind(spread) num_threads(numa_nodes)
        {
            const auto range = X.get_range(omp_get_place_num());
            #pragma omp parallel for proc_bind(master) num_threads(thrds_per_node)
            for (size_t i = range.first; i < range.second; i++){
                Y[i] = alpha * X[i] + Y[i];
            }
        }
        benchmark::ClobberMemory();
    }
}

static void Tbb(benchmark::State& state) {
    numa::ArenaMgtTBB arena;
    numa_extensions<ContainerType> X(arena.get_nodes(), state.range(0), 1);
    numa_extensions<ContainerType> Y(arena.get_nodes(), state.range(0), 1);

    constexpr ValueType alpha = 2;

    Partitioner part;

    for (auto _ : state) {
        arena.execute([&] (const int i) {
            auto range = X.get_range(i);
            tbb::parallel_for(tbb::blocked_range<size_t>(range.first, range.second), [&] (const tbb::blocked_range<size_t> r) {
                #pragma omp simd
                for (auto j = r.begin(); j < r.end(); j++) {
                    Y[j] = alpha * X[j] + Y[j];
                }
            }, part);
        });
    }
}

static void Tbb2(benchmark::State& state) {
    numa::ArenaMgtTBB arena;
    numa_extensions<ContainerType> X(arena, state.range(0), 1);
    numa_extensions<ContainerType> Y(arena, state.range(0), 1);

    constexpr ValueType alpha = 2;

    Partitioner part;

    for (auto _ : state) {
        arena.execute([&, alpha] (const int i) {
            auto range = X.get_range(i);
            tbb::parallel_for(tbb::blocked_range<size_t>(range.first, range.second), [&] (const tbb::blocked_range<size_t> r) {
                #pragma omp simd
                for (auto j = r.begin(); j < r.end(); j++) {
                    Y[j] = alpha * X[j] + Y[j];
                }
            }, part);
        });
    }
}

static void TbbSIMD(benchmark::State& state) {
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>;
    numa::ArenaMgtTBB arena;
    numa_extensions<ContainerTypeAligned> X(arena.get_nodes(), state.range(0), 1);
    numa_extensions<ContainerTypeAligned> Y(arena.get_nodes(), state.range(0), 1);

    using b_type = xsimd::batch<ValueType>;
    size_t inc = b_type::size;

    constexpr ValueType alpha = 2;

    Partitioner part;

    for (auto _ : state) {
        arena.execute([&] (const int i) {
            auto range = X.get_range(i);
            tbb::parallel_for(range.first, range.second, inc,  [&] (const size_t& j) {
                b_type x_vec = b_type::load_aligned(&X[j]);
                b_type y_vec = b_type::load_aligned(&Y[j]);
                b_type a_vec = b_type(alpha);

                b_type r_vec = xsimd::fma(a_vec, x_vec, y_vec);
                r_vec.store_aligned(&Y[j]);
            }, part);
        });
    }
}

static void TbbSIMD2(benchmark::State& state) {
    using ContainerTypeAligned = std::vector<ValueType, numa::no_init_allocator<ValueType, xsimd::aligned_allocator<ValueType>>>; 
    numa::ArenaMgtTBB arena;
    numa_extensions<ContainerTypeAligned> X(arena, state.range(0), 1);
    numa_extensions<ContainerTypeAligned> Y(arena, state.range(0), 1);

    using b_type = xsimd::batch<ValueType>;
    size_t inc = b_type::size;

    constexpr ValueType alpha = 2;
    Partitioner part;

    for (auto _ : state) {
        arena.execute([&] (const int i) {
            auto range = X.get_range(i);
            tbb::parallel_for(range.first, range.second, inc, [&] (const size_t& j) {
                b_type x_vec = b_type::load_aligned(&X[j]);
                b_type y_vec = b_type::load_aligned(&Y[j]);
                b_type a_vec = b_type(alpha);

                b_type r_vec = xsimd::fma(a_vec, x_vec, y_vec);
                r_vec.store_aligned(&Y[j]);
            }, part);
        });
    }
}


BENCHMARK(DefaultOmp)->Apply(Args)->UseRealTime();
// BENCHMARK(DefaultTbb)->Apply(Args)->UseRealTime();
// BENCHMARK(Omp)->Apply(Args)->UseRealTime();
// BENCHMARK(Omp2)->Apply(Args)->UseRealTime();
// BENCHMARK(OmpSIMD)->Apply(Args)->UseRealTime();
// BENCHMARK(OmpSIMD2)->Apply(Args)->UseRealTime();
// BENCHMARK(OmpNesting)->Apply(Args)->UseRealTime();
// BENCHMARK(OmpNesting2)->Apply(Args)->UseRealTime();
// BENCHMARK(Tbb)->Apply(Args)->UseRealTime();
// BENCHMARK(Tbb2)->Apply(Args)->UseRealTime();
// BENCHMARK(TbbSIMD)->Apply(Args)->UseRealTime();
// BENCHMARK(TbbSIMD2)->Apply(Args)->UseRealTime();
BENCHMARK_MAIN();