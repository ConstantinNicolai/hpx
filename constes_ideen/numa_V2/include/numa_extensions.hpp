#pragma once

#include <vector>
#include <utility>
#include <concepts>
#include <ranges>
#include <execution>
#include <omp.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/blocked_range.h>

#include <iostream>

#include "allocator_adaptor.hpp"
#include "arena.hpp"

template <typename Container>
concept container = std::random_access_iterator<typename Container::iterator> || std::contiguous_iterator<typename Container::iterator> || 
                    std::ranges::view<Container>;

template <container Container>
class numa_extensions {
public:
    using container_type = Container;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;

    // constructors

    numa_extensions() : container_(Container()) { }

    explicit numa_extensions(size_type count) : container_(Container(count)) { }

    explicit numa_extensions(size_type count, const value_type& value) : container_(Container(count)) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; i++) {
            new(&container_[i]) value_type{value};
        }        
    }
    
    explicit numa_extensions(numa::ArenaMgtTBB& arena, size_type count, const value_type& value) : container_(Container(count)) {
        node_ranges(count, arena.get_nodes());
        arena.execute([&] (const int i) {
            tbb::parallel_for(tbb::blocked_range<size_t>(node_range[i].first, node_range[i].second), [&] (const tbb::blocked_range<size_t> r) {
                #pragma omp simd
                for (auto i = r.begin(); i < r.end(); i++) {
                    new(&container_[i]) value_type{value};
                }
            }, tbb::static_partitioner());
        });
    }
    
    explicit numa_extensions(int nodes, size_type count, const value_type& value) : container_(Container(count)) {
        node_ranges(count, nodes);
        #pragma omp parallel proc_bind(spread) num_threads(nodes)
        {
            auto node = omp_get_place_num();
            std::uninitialized_fill(std::execution::unseq, container_.begin() + node_range[node].first, container_.begin() + node_range[node].second, value);
        }
    }

    explicit numa_extensions(int nodes, int thrds_per_node, size_type count, const value_type& value) : container_(Container(count)) {
        node_ranges(count, nodes);
        #pragma omp parallel proc_bind(spread) num_threads(nodes)
        {
            const auto range = node_range[omp_get_place_num()];
            #pragma omp parallel for proc_bind(master) num_threads(thrds_per_node)
            for (size_t i = range.first; i < range.second; i++) {
                new(&container_[i]) value_type{value};
            }
        }
    }

    void scan_init(size_type block_size, const value_type& init) {
        int threads = omp_get_num_procs();
        size_t iterations = container_.size() / (block_size * threads);
        size_t rest = container_.size() % (block_size * threads);
        int rest_threads = rest / block_size;
        int cores = threads / 2;

        #pragma omp parallel proc_bind(spread) num_threads(threads) firstprivate(iterations)
        {
            int tid = omp_get_thread_num();
            int block_index = (tid % 2) ? ((tid - 1) / 2 + cores) : (tid / 2);
            if (rest && block_index < rest_threads) iterations++;
            for (int i = 0; i < iterations; i++) {
                size_t start = i * threads * block_size + block_index * block_size;
                std::uninitialized_fill_n(&container_[start], block_size, init);
            }
        }
    }

    ~numa_extensions() = default;

    constexpr size_type size() const noexcept { return container_.size(); }

    constexpr value_type* data() noexcept { return container_.data(); }

    constexpr iterator begin() noexcept { return container_.begin(); }

    constexpr iterator end() noexcept { return container_.end(); }

    constexpr const_iterator cbegin() const noexcept { return container_.cbegin(); }

    constexpr const_iterator cend() const noexcept { return container_.cend(); }

    reference operator[](size_type index) { return container_[index]; }

    reference back() { return container_.back(); };

    std::pair<size_t, size_t> get_range(size_type index) { return node_range[index]; }

private:
    void node_ranges(size_type count, int nodes) {
        node_range.reserve(nodes);

        auto node_size = count / nodes;
        auto rest = count % nodes;

        size_t start = 0;
        for (auto i = 0; i < nodes; i++) {
            auto size = (i < rest) ? node_size + 1 : node_size; 
            node_range[i] = std::make_pair(start, start + size);
            start += size;
        }
    }

    Container container_;

    std::vector<std::pair<size_t, size_t>> node_range;

};
