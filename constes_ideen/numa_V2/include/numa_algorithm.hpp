#include <vector>
#include <functional>
#include <execution>
#include <atomic>
#include <future>
#include <iostream>
#include <iterator>
#include <benchmark/benchmark.h>
#include <omp.h>

#include "numa_extensions.hpp"
#include "allocator_adaptor.hpp"


template <class InputIt, class OutputIt,
          class BinaryOp, class T>
void numa_scan(InputIt first, InputIt last, OutputIt d_first, BinaryOp binary_op, T init, size_t block_size) {
    size_t size = std::distance(first, last);
    int threads = omp_get_num_procs();
    int cores = threads / 2;
    if (block_size > (size / threads)) block_size = size / threads;
    
    std::vector<T, numa::no_init_allocator<T>> aux1(cores);
    std::vector<T, numa::no_init_allocator<T>> aux2(cores);
    std::vector<std::future<T>, numa::no_init_allocator<std::future<T>>> futures(threads);
    std::vector<std::promise<T>, numa::no_init_allocator<std::promise<T>>> promises(threads);
    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};

    size_t iterations = size / (block_size * threads);
    size_t rest = size % (block_size * threads);
    int rest_threads = rest / block_size;
  
    #pragma omp parallel proc_bind(spread) num_threads(threads) firstprivate(iterations)
    {
        int tid = omp_get_thread_num();
        bool hyperthread = tid % 2; 
        auto aux_ptr = hyperthread ? &aux2 : &aux1;
        auto counter_ptr = hyperthread ? &counter2 : &counter1;
        int block_index = hyperthread ? ((tid - 1) / 2 + cores) : (tid / 2);
        int aux_index = hyperthread ? ((tid - 1 ) / 2) : (tid / 2);
        int prom_index = hyperthread ? ((tid - 1) / 2) : (tid / 2) + cores;
        T start_value = 0;
        T transfer_in = 0;
        T transfer_out = 0;
        new(&promises[block_index]) std::promise<T>{};
        new(&futures[block_index]) std::future<T>(std::move(promises[block_index].get_future()));
        new(&aux_ptr->operator[](aux_index)) T{0};

        #pragma omp barrier

        int last_count = cores;
        int last = -1;
        if (rest && block_index < rest_threads) {
            if (hyperthread && rest_threads > cores) {
                last_count = rest_threads % cores;
            }
            else if (!hyperthread && rest_threads < cores) {
                last_count = rest_threads % cores;
            }
            last = iterations;
            iterations++;
        }
        for (int i = 0; i < iterations; i++) {
            size_t start = i * threads * block_size + block_index * block_size;
            T block_result = std::reduce(std::execution::unseq, first + start, first + start + block_size, T{0}, binary_op);
            aux_ptr->operator[](aux_index) = block_result;
            counter_ptr->fetch_add(1, std::memory_order::release);
            
            int count_val = (i == last) ? i * cores + last_count : (i + 1) * cores;
            while (counter_ptr->load(std::memory_order::relaxed) < count_val) { std::this_thread::yield(); }
            
            start_value = std::reduce(std::execution::unseq, &aux_ptr->operator[](0), &aux_ptr->operator[](aux_index), T{0}, binary_op);
            transfer_out = std::reduce(std::execution::unseq, aux_ptr->begin(), aux_ptr->end(), T{0}, binary_op);
            if (i > 0 || hyperthread) {
                futures[block_index].wait();
                transfer_in = futures[block_index].get();
                promises[block_index] = std::promise<T>{};
                futures[block_index] = promises[block_index].get_future();
                transfer_out += transfer_in;
                start_value += transfer_in;
            }
            promises[prom_index].set_value(transfer_out);
            std::inclusive_scan(std::execution::unseq, first + start, first + start + block_size, first + start, binary_op, start_value + init);
        }
    }
}
