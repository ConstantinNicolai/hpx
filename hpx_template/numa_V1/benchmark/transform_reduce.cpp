#include <hpx/hpx_init.hpp>
#include <hpx/include/parallel_algorithm.hpp>
#include <hpx/include/parallel_numeric.hpp>
#include <hpx/include/parallel_sort.hpp>
#include <hpx/include/parallel_transform_reduce.hpp>
#include <hpx/memory/locality_allocator.hpp>

int hpx_main(int argc, char* argv[])
{
    // Allocate 100 integers on the local NUMA node
    hpx::memory::locality_allocator<int> alloc(hpx::find_here());
    std::vector<int, hpx::memory::locality_allocator<int>> v(alloc);
    v.resize(100);

    // Fill the vector with random numbers
    std::generate(v.begin(), v.end(), std::rand);

    // Sort the vector
    hpx::parallel::sort(hpx::parallel::execution::par, v.begin(), v.end());

    // Compute the sum of the vector
    int sum = hpx::parallel::transform_reduce(
        hpx::parallel::execution::par, v.begin(), v.end(),
        [](int x) { return x; }, 0,
        std::plus<int>());

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}